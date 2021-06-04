#include "player.h"
#include "draw_command.h"
#include "healthbar.h"
#include "item_catalog.h"
#include "maths.h"
#include "shadow.h"
#include "sprite.h"
#include "spritesheet.h"
#include <cassert>

void player_init(Player *player, const char *name, const struct SpriteDef *spriteDef)
{
    assert(player);
    assert(name);
    assert(spriteDef);

    player->name = name;
    player->actionState = PlayerActionState_None;
    player->moveState = PlayerMoveState_Idle;
    player->body.lastUpdated = GetTime();
    player->sprite.spriteDef = spriteDef;
    player->sprite.scale = 1.0f;
    player->sprite.direction = Direction_South;
    player->combat.maxHitPoints = 100.0f;
    player->combat.hitPoints = player->combat.maxHitPoints;
    player->combat.meleeDamage = 1.0f;

    // TODO: Load selected slot from save file / server
    player->inventory.selectedSlot = PlayerInventorySlot_1;

    // TODO: Load inventory from save file / server
    player->inventory.slots[PlayerInventorySlot_1    ] = { ItemID_Weapon_Sword ,  1 };
    player->inventory.slots[PlayerInventorySlot_Coins] = { ItemID_Currency_Coin,  0 };

    // TODO: Load stats from save file / server
    //player->stats.coinsCollected = 33;
    //player->stats.slimesSlain = 44;
}

Vector3 player_get_attach_point(const Player *player, PlayerAttachPoint attachPoint)
{
    Vector3 attach = {};
    switch (attachPoint) {
        case PlayerAttachPoint_Gut: {
            Vector3 playerC = sprite_world_center(&player->sprite, player->body.position, player->sprite.scale);
            attach = v3_add(playerC, { 0.0f, 0.0f, -10.0f });
            break;
        } default: {
            assert(!"That's not a valid attachment point identifier");
            break;
        }
    }
    return attach;
}

const Item *player_selected_item(const Player *player)
{
    const ItemStack *selectedStack = &player->inventory.slots[player->inventory.selectedSlot];
    const Item *selectedItem = item_catalog_find(selectedStack->id);
    return selectedItem;
}

static void update_direction(Player *player, Vector2 offset)
{
    // NOTE: Branching could be removed by putting the sprites in a more logical order.. doesn't matter if this
    // only applies to players since there would be so few.
    Direction prevDirection = player->sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            player->sprite.direction = Direction_SouthEast;
        } else if (offset.y < 0.0f) {
            player->sprite.direction = Direction_NorthEast;
        } else {
            player->sprite.direction = Direction_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            player->sprite.direction = Direction_SouthWest;
        } else if (offset.y < 0.0f) {
            player->sprite.direction = Direction_NorthWest;
        } else {
            player->sprite.direction = Direction_West;
        }
    } else {
        if (offset.y > 0.0f) {
            player->sprite.direction = Direction_South;
        } else if (offset.y < 0.0f) {
            player->sprite.direction = Direction_North;
        }
    }
    if (player->sprite.direction != prevDirection) {
        player->sprite.animFrameIdx = 0;
    }
}

bool player_move(Player *player, double now, double dt, Vector2 offset)
{
    assert(player);
    if (v2_is_zero(offset))
        return false;

    // Don't allow player to move if they're not touching the ground (currently no jump/fall, so just assert)
    assert(player->body.position.z == 0.0f);

    player->body.position.x += offset.x;
    player->body.position.y += offset.y;
    update_direction(player, offset);

    const float pixelsMoved = v2_length(offset);
    const float metersMoved = PIXELS_TO_METERS(pixelsMoved);
    player->stats.kmWalked += metersMoved / 1000.0f;
    return true;
}

bool player_attack(Player *player, double now, double dt)
{
    if (player->actionState == PlayerActionState_None) {
        player->actionState = PlayerActionState_Attacking;
        player->body.lastUpdated = now;
        player->combat.attackStartedAt = now;
        player->combat.attackDuration = 0.1;

        const Item *selectedItem = player_selected_item(player);
        switch (selectedItem->id) {
            case ItemID_Weapon_Sword: {
                player->stats.timesSwordSwung++;
                break;
            }
            default: {
                player->stats.timesFistSwung++;
                break;
            }
        }

        return true;
    }
    return false;
}

void player_update(Player *player, double now, double dt)
{
    const double timeSinceAttackStarted = now - player->combat.attackStartedAt;
    if (timeSinceAttackStarted > player->combat.attackDuration) {
        player->actionState = PlayerActionState_None;
        player->combat.attackStartedAt = 0;
        player->combat.attackDuration = 0;
    }

    // TODO: Less hard-coded way to look up player sprite based on selected item id
    Spritesheet *sheet = player->sprite.spriteDef->spritesheet;
    assert(sheet->spriteCount == 5);

    const Item *selectedItem = player_selected_item(player);
    if (selectedItem->id == ItemID_Weapon_Sword) {
        switch (player->actionState) {
            case PlayerActionState_None: {
                switch (player->body.idle) {
                    // TODO: sprite_by_name("player_sword");
                    case false: player->sprite.spriteDef = &sheet->sprites[2]; break;
                        // TODO: sprite_by_name("player_sword_idle");
                    case true:  player->sprite.spriteDef = &sheet->sprites[3]; break;
                }
                break;
            }
            case PlayerActionState_Attacking: {
                // sprite_by_name("player_sword_attack");
                player->sprite.spriteDef = &sheet->sprites[4];
                break;
            }
        }
    } else {
        switch (player->body.idle) {
            // TODO: sprite_by_name("player_melee");
            case false: player->sprite.spriteDef = &sheet->sprites[0]; break;
                // TODO: sprite_by_name("player_melee_idle");
            case true:  player->sprite.spriteDef = &sheet->sprites[1]; break;
        }
    }

    body_update(&player->body, now, dt);
    sprite_update(&player->sprite, now, dt);
}

float player_depth(const Player *player)
{
    const float depth = player->body.position.y;
    return depth;
}

bool player_cull(const Player* player, Rectangle cullRect)
{
    bool cull = sprite_cull_body(&player->sprite, &player->body, cullRect);
    return cull;
}

void player_push(const Player *player)
{
    draw_command_push(DrawableType_Player, player);
}

void player_draw(const Player *player)
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;
    const Vector2 playerGroundPos = body_ground_position(&player->body);
    shadow_draw((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

    sprite_draw_body(&player->sprite, &player->body, WHITE);
    healthbar_draw(10, &player->sprite, &player->body, player->combat.hitPoints, player->combat.maxHitPoints);
}