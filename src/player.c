#include "player.h"
#include "spritesheet.h"
#include "math.h"
#include <assert.h>

void player_init(Player *player, const char *name, struct Sprite *sprite)
{
    assert(player);
    assert(name);
    assert(sprite);

    player->name = name;
    player->body.scale = 1.0f;
    player->body.lastUpdated = GetTime();
    player->body.facing = Facing_South;
    player->body.color = WHITE;
    player->body.sprite = sprite;
    player->combat.maxHitPoints = 100.0f;
    player->combat.hitPoints = player->combat.maxHitPoints;
}

Vector3 player_get_attach_point(const Player *player, PlayerAttachPoint attachPoint)
{
    Vector3 attach = { 0 };
    switch (attachPoint) {
        case PlayerAttachPoint_Gut: {
            attach = v3_add(body_center(&player->body), (Vector3){ 0.0f, 0.0f, -10.0f });
            break;
        } default: {
            assert(!"That's not a valid attachment point identifier");
            break;
        }
    }
    return attach;
}

static void update_direction(Player *player, Vector2 offset)
{
    // NOTE: Branching could be removed by putting the sprites in a more logical order.. doesn't matter if this
    // only applies to players since there would be so few.
    Facing prevFacing = player->body.facing;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            player->body.facing = Facing_SouthEast;
        } else if (offset.y < 0.0f) {
            player->body.facing = Facing_NorthEast;
        } else {
            player->body.facing = Facing_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            player->body.facing = Facing_SouthWest;
        } else if (offset.y < 0.0f) {
            player->body.facing = Facing_NorthWest;
        } else {
            player->body.facing = Facing_West;
        }
    } else {
        if (offset.y > 0.0f) {
            player->body.facing = Facing_South;
        } else if (offset.y < 0.0f) {
            player->body.facing = Facing_North;
        }
    }
    if (player->body.facing != prevFacing) {
        player->body.animFrameIdx = 0;
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
    return true;
}

bool player_attack(Player *player, double now, double dt)
{
    if (player->combat.weapon && player->action == PlayerAction_None) {
        player->action = PlayerAction_Attack;
        player->body.lastUpdated = now;
        player->combat.attackStartedAt = now;
        player->combat.attackDuration = 0.1;
        return true;
    }
    return false;
}

void player_update(Player *player, double now, double dt)
{
    const double timeSinceLastMove = now - player->body.lastUpdated;
    if (timeSinceLastMove > 6.0) {
        // TODO: Find a better way to handle idling than overwriting the facing direction
        //player->body.facing = Facing_Idle;
    }

    const double timeSinceAttackStarted = now - player->combat.attackStartedAt;
    if (timeSinceAttackStarted > player->combat.attackDuration) {
        player->action = PlayerAction_None;
        player->combat.attackStartedAt = 0;
        player->combat.attackDuration = 0;
    }

    // TODO: Make this suck less
    Spritesheet *sheet = player->body.sprite->spritesheet;
    assert(sheet->spriteCount == 3);
    if (player->combat.weapon->damage == 1.0f) {
        // TODO: If not found, set sprite to null and draw a red rectangle
        // sprite_by_name("player_melee");
        player->body.sprite = &sheet->sprites[0];
    } else {
        switch (player->action) {
            case PlayerAction_None: {
                // sprite_by_name("player_sword");
                player->body.sprite = &sheet->sprites[1];
                break;
            }
            case PlayerAction_Attack: {
                // sprite_by_name("player_sword_attack");
                player->body.sprite = &sheet->sprites[2];
                break;
            }
        }
    }

    body_update(&player->body, now, dt);
}

void player_draw(Player *player)
{
    body_draw(&player->body);
}