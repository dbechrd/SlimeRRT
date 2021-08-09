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

    this->name = name;
    actionState = PlayerActionState_None;
    moveState = PlayerMoveState_Idle;
    body.lastUpdated = GetTime();
    sprite.spriteDef = &spriteDef;
    sprite.scale = 1.0f;
    sprite.direction = Direction_South;
    combat.maxHitPoints = 100.0f;
    combat.hitPoints = combat.maxHitPoints;
    combat.meleeDamage = 1.0f;

    // TODO: Load selected slot from save file / server
    inventory.selectedSlot = PlayerInventorySlot_1;

    // TODO: Load inventory from save file / server
    inventory.slots[PlayerInventorySlot_1    ] = { ItemID_Weapon_Sword ,  1 };
    inventory.slots[PlayerInventorySlot_Coins] = { ItemID_Currency_Coin,  0 };

    // TODO: Load stats from save file / server
    //player->stats.coinsCollected = 33;
    //player->stats.slimesSlain = 44;
}

Vector3 Player::GetAttachPoint(PlayerAttachPoint attachPoint) const
{
    Vector3 attach = {};
    switch (attachPoint) {
        case PlayerAttachPoint_Gut: {
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
    const ItemStack &selectedStack = inventory.slots[inventory.selectedSlot];
    const Item &selectedItem = item_catalog_find(selectedStack.id);
    return selectedItem;
}

void Player::UpdateDirection(Vector2 offset)
{
    // NOTE: Branching could be removed by putting the sprites in a more logical order.. doesn't matter if this
    // only applies to players since there would be so few.
    Direction prevDirection = sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = Direction_SouthEast;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction_NorthEast;
        } else {
            sprite.direction = Direction_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = Direction_SouthWest;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction_NorthWest;
        } else {
            sprite.direction = Direction_West;
        }
    } else {
        if (offset.y > 0.0f) {
            sprite.direction = Direction_South;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction_North;
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

    if (actionState == PlayerActionState_None) {
        actionState = PlayerActionState_Attacking;
        body.lastUpdated = now;
        combat.attackStartedAt = now;
        combat.attackDuration = 0.1;

        const Item &selectedItem = GetSelectedItem();
        switch (selectedItem.id) {
            case ItemID_Weapon_Sword: {
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
        actionState = PlayerActionState_None;
        combat.attackStartedAt = 0;
        combat.attackDuration = 0;
    }

    // TODO: Less hard-coded way to look up player sprite based on selected item id
    const Spritesheet *sheet = sprite.spriteDef->spritesheet;
    assert(sheet->sprites.size() == 5);

    const Item &selectedItem = GetSelectedItem();
    if (selectedItem.id == ItemID_Weapon_Sword) {
        switch (actionState) {
            case PlayerActionState_None: {
                switch (body.idle) {
                    // TODO: sprite_by_name("player_sword");
                    case false: sprite.spriteDef = &sheet->sprites[2]; break;
                        // TODO: sprite_by_name("player_sword_idle");
                    case true:  sprite.spriteDef = &sheet->sprites[3]; break;
                }
                break;
            }
            case PlayerActionState_Attacking: {
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

    body_update(&body, now, dt);
    sprite_update(sprite, now, dt);
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

void Player::Push() const
{
    draw_command_push(DrawableType_Player, this);
}

void Player::Draw() const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;
    const Vector2 playerGroundPos = body_ground_position(&body);
    shadow_draw((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

    sprite_draw_body(sprite, body, WHITE);
    healthbar_draw(10, sprite, body, combat.hitPoints, combat.maxHitPoints);
}