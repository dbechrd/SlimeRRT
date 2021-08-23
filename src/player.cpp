#include "player.h"
#include "draw_command.h"
#include "healthbar.h"
#include "item_catalog.h"
#include "maths.h"
#include "shadow.h"
#include "sprite.h"
#include "spritesheet.h"
#include <cassert>

Player::Player(const char *playerName, const SpriteDef &spriteDef) : Player()
{
    assert(playerName);

    name = playerName;
    actionState = ActionState::None;
    moveState = MoveState::Idle;
    body.lastUpdated = GetTime();
    sprite.spriteDef = &spriteDef;
    sprite.scale = 1.0f;
    sprite.direction = Direction::South;
    combat.maxHitPoints = 100.0f;
    combat.hitPoints = combat.maxHitPoints;
    combat.meleeDamage = 1.0f;

    // TODO: Load selected slot from save file / server
    inventory.selectedSlot = PlayerInventorySlot::Slot_1;

    // TODO: Load inventory from save file / server
    inventory.slots[(int)PlayerInventorySlot::Slot_1] = { ItemID::Weapon_Sword ,  1 };
    inventory.slots[(int)PlayerInventorySlot::Coins ] = { ItemID::Currency_Coin,  0 };

    // TODO: Load stats from save file / server
    //stats.coinsCollected = 33;
    //stats.slimesSlain = 44;
    stats = {};
}

float Player::Depth() const
{
    return body.position.y;
}

bool Player::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void Player::Draw() const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;
    const Vector2 playerGroundPos = body.GroundPosition();
    Shadow::Draw((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

    sprite_draw_body(sprite, body, WHITE);
    HealthBar::Draw(10, sprite, body, combat.hitPoints, combat.maxHitPoints);
}

Vector3 Player::GetAttachPoint(AttachPoint attachPoint) const
{
    Vector3 attach = {};
    switch (attachPoint) {
        case AttachPoint::Gut: {
            Vector3 playerC = sprite_world_center(sprite, body.position, sprite.scale);
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
    const ItemStack &selectedStack = inventory.slots[(int)inventory.selectedSlot];
    const Item &selectedItem = g_itemCatalog.FindById(selectedStack.id);
    return selectedItem;
}

void Player::UpdateDirection(Vector2 offset)
{
    // NOTE: Branching could be removed by putting the sprites in a more logical order.. doesn't matter if this
    // only applies to players since there would be so few.
    Direction prevDirection = sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = Direction::SouthEast;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction::NorthEast;
        } else {
            sprite.direction = Direction::East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = Direction::SouthWest;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction::NorthWest;
        } else {
            sprite.direction = Direction::West;
        }
    } else {
        if (offset.y > 0.0f) {
            sprite.direction = Direction::South;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction::North;
        }
    }
    if (sprite.direction != prevDirection) {
        sprite.animFrameIdx = 0;
    }
}

bool Player::Move(double now, double dt, Vector2 offset)
{
    UNUSED(now);
    UNUSED(dt);  // offset is already in time slice space

    if (v2_is_zero(offset))
        return false;

    // Don't allow player to move if they're not touching the ground (currently no jump/fall, so just assert)
    assert(body.position.z == 0.0f);

    body.position.x += offset.x;
    body.position.y += offset.y;
    UpdateDirection(offset);

    const float pixelsMoved = v2_length(offset);
    const float metersMoved = PIXELS_TO_METERS(pixelsMoved);
    stats.kmWalked += metersMoved / 1000.0f;
    return true;
}

bool Player::Attack(double now, double dt)
{
    UNUSED(dt);

    if (actionState == ActionState::None) {
        actionState = ActionState::Attacking;
        body.lastUpdated = now;
        combat.attackStartedAt = now;
        combat.attackDuration = 0.1;

        const Item &selectedItem = GetSelectedItem();
        switch (selectedItem.id) {
            case ItemID::Weapon_Sword: {
                stats.timesSwordSwung++;
                break;
            }
            default: {
                stats.timesFistSwung++;
                break;
            }
        }

        return true;
    }
    return false;
}

void Player::Update(double now, double dt)
{
    const double timeSinceAttackStarted = now - combat.attackStartedAt;
    if (timeSinceAttackStarted > combat.attackDuration) {
        actionState = ActionState::None;
        combat.attackStartedAt = 0;
        combat.attackDuration = 0;
    }

    // TODO: Less hard-coded way to look up player sprite based on selected item id
    const Spritesheet *sheet = sprite.spriteDef->spritesheet;
    assert(sheet->sprites.size() == 5);

    const Item &selectedItem = GetSelectedItem();
    if (selectedItem.id == ItemID::Weapon_Sword) {
        switch (actionState) {
            case ActionState::None: {
                switch (body.idle) {
                    // TODO: sprite_by_name("player_sword");
                    case false: sprite.spriteDef = &sheet->sprites[2]; break;
                        // TODO: sprite_by_name("player_sword_idle");
                    case true:  sprite.spriteDef = &sheet->sprites[3]; break;
                }
                break;
            }
            case ActionState::Attacking: {
                // sprite_by_name("player_sword_attack");
                sprite.spriteDef = &sheet->sprites[4];
                break;
            }
        }
    } else {
        switch (body.idle) {
            // TODO: sprite_by_name("player_melee");
            case false: sprite.spriteDef = &sheet->sprites[0]; break;
                // TODO: sprite_by_name("player_melee_idle");
            case true:  sprite.spriteDef = &sheet->sprites[1]; break;
        }
    }

    body.Update(now, dt);
    sprite_update(sprite, now, dt);
}