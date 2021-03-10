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
    player->body.lastMoveTime = GetTime();
    player->body.facing = Facing_Idle;
    player->body.alpha = 1.0f;
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
}

bool player_move(Player *player, Vector2 offset)
{
    assert(player);
    if (v2_is_zero(offset))
        return false;

    // Don't allow player to move if they're not touching the ground (currently no jump/fall, so just assert)
    assert(player->body.position.z == 0.0f);

    player->body.position.x += offset.x;
    player->body.position.y += offset.y;
    player->body.lastMoveTime = GetTime();
    update_direction(player, offset);
    return true;
}

bool player_attack(Player *player)
{
    if (player->combat.weapon && player->action == PlayerAction_None) {
        player->action = PlayerAction_Attack;
        player->body.lastMoveTime = GetTime();
        player->combat.attackStartedAt = GetTime();
        player->combat.attackDuration = 0.1;
        return true;
    }
    return false;
}

void player_update(Player *player)
{
    const double t = GetTime();

    const double timeSinceLastMove = t - player->body.lastMoveTime;
    if (timeSinceLastMove > 60.0) {
        player->body.facing = Facing_Idle;
    }

    const double timeSinceAttackStarted = t - player->combat.attackStartedAt;
    if (timeSinceAttackStarted > player->combat.attackDuration) {
        player->action = PlayerAction_None;
        player->combat.attackStartedAt = 0;
        player->combat.attackDuration = 0;
    }

    // TODO: Set player->sprite to the correct sprite more intelligently?
    // TODO: Could use bitmask with & on the various states / flags
    int frameIdx = player->body.facing;
    frameIdx += (player->combat.weapon ? 1 : 0) * Facing_Count;
    if (player->combat.weapon) {
        frameIdx += player->action * Facing_Count;
    }

    player->body.spriteFrameIdx = (size_t)frameIdx;
}

void player_draw(Player *player)
{
    body_draw(&player->body);
}