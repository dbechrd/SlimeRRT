#include "player.h"
#include "draw_command.h"
#include "healthbar.h"
#include "item_catalog.h"
#include "maths.h"
#include "shadow.h"
#include "sprite.h"
#include "spritesheet.h"
#include <cassert>

Player::Player(const char *name, const SpriteDef &spriteDef) : Player()
{
    assert(name);

    m_name = name;
    m_actionState = ActionState::None;
    m_moveState = MoveState::Idle;
    m_body.lastUpdated = GetTime();
    m_sprite.spriteDef = &spriteDef;
    m_sprite.scale = 1.0f;
    m_sprite.direction = Direction::South;
    m_combat.maxHitPoints = 100.0f;
    m_combat.hitPoints = m_combat.maxHitPoints;
    m_combat.meleeDamage = 1.0f;

    // TODO: Load selected slot from save file / server
    m_inventory.selectedSlot = PlayerInventorySlot::Slot_1;

    // TODO: Load inventory from save file / server
    m_inventory.slots[(int)PlayerInventorySlot::Slot_1] = { ItemID::Weapon_Sword ,  1 };
    m_inventory.slots[(int)PlayerInventorySlot::Coins ] = { ItemID::Currency_Coin,  0 };

    // TODO: Load stats from save file / server
    //m_stats.coinsCollected = 33;
    //m_stats.slimesSlain = 44;
    m_stats = {};
}

Vector3 Player::GetAttachPoint(AttachPoint attachPoint) const
{
    Vector3 attach = {};
    switch (attachPoint) {
        case AttachPoint::Gut: {
            Vector3 playerC = sprite_world_center(m_sprite, m_body.position, m_sprite.scale);
            attach = v3_add(playerC, { 0.0f, 0.0f, -10.0f });
            break;
        } default: {
            assert(!"That's not a valid attachment point identifier");
            break;
        }
    }
    return attach;
}

const Item &Player::GetSelectedItem() const
{
    const ItemStack &selectedStack = m_inventory.slots[(int)m_inventory.selectedSlot];
    const Item &selectedItem = item_catalog_find(selectedStack.id);
    return selectedItem;
}

void Player::UpdateDirection(Vector2 offset)
{
    // NOTE: Branching could be removed by putting the sprites in a more logical order.. doesn't matter if this
    // only applies to players since there would be so few.
    Direction prevDirection = m_sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            m_sprite.direction = Direction::SouthEast;
        } else if (offset.y < 0.0f) {
            m_sprite.direction = Direction::NorthEast;
        } else {
            m_sprite.direction = Direction::East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            m_sprite.direction = Direction::SouthWest;
        } else if (offset.y < 0.0f) {
            m_sprite.direction = Direction::NorthWest;
        } else {
            m_sprite.direction = Direction::West;
        }
    } else {
        if (offset.y > 0.0f) {
            m_sprite.direction = Direction::South;
        } else if (offset.y < 0.0f) {
            m_sprite.direction = Direction::North;
        }
    }
    if (m_sprite.direction != prevDirection) {
        m_sprite.animFrameIdx = 0;
    }
}

bool Player::Move(double now, double dt, Vector2 offset)
{
    UNUSED(now);
    UNUSED(dt);  // offset is already in time slice space

    if (v2_is_zero(offset))
        return false;

    // Don't allow player to move if they're not touching the ground (currently no jump/fall, so just assert)
    assert(m_body.position.z == 0.0f);

    m_body.position.x += offset.x;
    m_body.position.y += offset.y;
    UpdateDirection(offset);

    const float pixelsMoved = v2_length(offset);
    const float metersMoved = PIXELS_TO_METERS(pixelsMoved);
    m_stats.kmWalked += metersMoved / 1000.0f;
    return true;
}

bool Player::Attack(double now, double dt)
{
    UNUSED(dt);

    if (m_actionState == ActionState::None) {
        m_actionState = ActionState::Attacking;
        m_body.lastUpdated = now;
        m_combat.attackStartedAt = now;
        m_combat.attackDuration = 0.1;

        const Item &selectedItem = GetSelectedItem();
        switch (selectedItem.id) {
            case ItemID::Weapon_Sword: {
                m_stats.timesSwordSwung++;
                break;
            }
            default: {
                m_stats.timesFistSwung++;
                break;
            }
        }

        return true;
    }
    return false;
}

void Player::Update(double now, double dt)
{
    const double timeSinceAttackStarted = now - m_combat.attackStartedAt;
    if (timeSinceAttackStarted > m_combat.attackDuration) {
        m_actionState = ActionState::None;
        m_combat.attackStartedAt = 0;
        m_combat.attackDuration = 0;
    }

    // TODO: Less hard-coded way to look up player sprite based on selected item id
    const Spritesheet *sheet = m_sprite.spriteDef->spritesheet;
    assert(sheet->sprites.size() == 5);

    const Item &selectedItem = GetSelectedItem();
    if (selectedItem.id == ItemID::Weapon_Sword) {
        switch (m_actionState) {
            case ActionState::None: {
                switch (m_body.idle) {
                    // TODO: sprite_by_name("player_sword");
                    case false: m_sprite.spriteDef = &sheet->sprites[2]; break;
                        // TODO: sprite_by_name("player_sword_idle");
                    case true:  m_sprite.spriteDef = &sheet->sprites[3]; break;
                }
                break;
            }
            case ActionState::Attacking: {
                // sprite_by_name("player_sword_attack");
                m_sprite.spriteDef = &sheet->sprites[4];
                break;
            }
        }
    } else {
        switch (m_body.idle) {
            // TODO: sprite_by_name("player_melee");
            case false: m_sprite.spriteDef = &sheet->sprites[0]; break;
                // TODO: sprite_by_name("player_melee_idle");
            case true:  m_sprite.spriteDef = &sheet->sprites[1]; break;
        }
    }

    m_body.Update(now, dt);
    sprite_update(m_sprite, now, dt);
}

float Player::Depth() const
{
    return m_body.position.y;
}

bool Player::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(m_sprite, m_body, cullRect);
    return cull;
}

void Player::Push() const
{
    draw_command_push(DrawableType::Player, this);
}

void Player::Draw() const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;
    const Vector2 playerGroundPos = m_body.GroundPosition();
    Shadow::Draw((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

    sprite_draw_body(m_sprite, m_body, WHITE);
    HealthBar::Draw(10, m_sprite, m_body, m_combat.hitPoints, m_combat.maxHitPoints);
}