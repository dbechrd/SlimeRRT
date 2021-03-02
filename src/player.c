#include "player.h"
#include "spritesheet.h"
#include "vector2f.h"
#include <assert.h>

void player_init(Player *player, const char *name, struct Sprite *sprite)
{
    assert(player);
    assert(name);
    assert(sprite);

    player->name = name;
    player->facing = PlayerFacing_South;
    player->lastMoveTime = GetTime();
    player->maxHitPoints = 100.0f;
    player->hitPoints = player->maxHitPoints;
    player->sprite = sprite;
}

SpriteFrame *player_get_frame(Player *player)
{
    SpriteFrame *frame = &player->sprite->frames[player->spriteFrameIdx];
    return frame;
}

Rectangle player_get_frame_rect(Player *player)
{
    assert(player->spriteFrameIdx < player->sprite->frameCount);

    const SpriteFrame *frame = player_get_frame(player);
    Rectangle rect = { 0 };
    rect.x = (float)frame->x;
    rect.y = (float)frame->y;
    rect.width = (float)frame->width;
    rect.height = (float)frame->height;
    return rect;
}

Rectangle player_get_rect(Player *player)
{
    assert(player->spriteFrameIdx < player->sprite->frameCount);

    Rectangle frameRect = player_get_frame_rect(player);
    Rectangle rect = { 0 };
    rect.x = player->position.x;
    rect.y = player->position.y;
    rect.width = frameRect.width;
    rect.height = frameRect.height;
    return rect;
}

Vector2 player_get_center(Player *player)
{
    const SpriteFrame *spriteFrame = &player->sprite->frames[player->spriteFrameIdx];
    Vector2 center = { 0 };
    center.x = player->position.x + spriteFrame->width / 2.0f;
    center.y = player->position.y + spriteFrame->height / 2.0f;
    return center;
}

Vector2 player_get_bottom_center(Player *player)
{
    const SpriteFrame *spriteFrame = &player->sprite->frames[player->spriteFrameIdx];
    Vector2 center = { 0 };
    center.x = player->position.x + spriteFrame->width / 2.0f;
    center.y = player->position.y + spriteFrame->height;
    return center;
}

Vector2 player_get_attach_point(Player *player, PlayerAttachPoint attachPoint)
{
    Vector2 attach = { 0 };
    switch (attachPoint) {
        case PlayerAttachPoint_Gut: {
            attach = v2_add(player_get_center(player), (Vector2){ 0.0f, 10.0f });
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
            player->facing = PlayerFacing_SouthEast;
        } else if (offset.y < 0.0f) {
            player->facing = PlayerFacing_NorthEast;
        } else {
            player->facing = PlayerFacing_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            player->facing = PlayerFacing_SouthWest;
        } else if (offset.y < 0.0f) {
            player->facing = PlayerFacing_NorthWest;
        } else {
            player->facing = PlayerFacing_West;
        }
    } else {
        if (offset.y > 0.0f) {
            player->facing = PlayerFacing_South;
        } else if (offset.y < 0.0f) {
            player->facing = PlayerFacing_North;
        }
    }
}

void player_move(Player *player, Vector2 offset)
{
    assert(player);
    if (v2_is_zero(offset)) return;

    player->position = v2_add(player->position, offset);
    player->lastMoveTime = GetTime();
    update_direction(player, offset);
}

bool player_attack(Player *player)
{
    if (player->equippedWeapon && player->action == PlayerAction_None) {
        player->action = PlayerAction_Attack;
        player->lastMoveTime = GetTime();
        player->attackStartedAt = GetTime();
        player->attackDuration = 0.1;
        return true;
    }
    return false;
}

void player_update(Player *player)
{
    const double t = GetTime();

    const double timeSinceLastMove = t - player->lastMoveTime;
    if (timeSinceLastMove > 10.0) {
        player->facing = PlayerFacing_Idle;
    }

    const double timeSinceAttackStarted = t - player->attackStartedAt;
    if (timeSinceAttackStarted > player->attackDuration) {
        player->action = PlayerAction_None;
        player->attackStartedAt = 0;
        player->attackDuration = 0;
    }

    // TODO: Set player->sprite to the correct sprite more intelligently?
    // TODO: Could use bitmask with & on the various states / flags
    int frameIdx = player->facing;
    frameIdx += player->equippedWeapon * PlayerFacing_Count;
    if (player->equippedWeapon) {
        frameIdx += player->action * PlayerFacing_Count;
    }

    player->spriteFrameIdx = (size_t)frameIdx;
}

void player_draw(Player *player)
{
#if 0
    // Funny bug where texture stays still relative to screen, could be fun to abuse later
    const Rectangle rect = player_get_rect(player);
#endif
    const Rectangle rect = player_get_frame_rect(player);
    DrawTextureRec(*player->sprite->spritesheet->texture, rect, player->position, WHITE);
}