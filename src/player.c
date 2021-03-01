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
    player->state = PlayerState_South;
    player->lastMoveTime = GetTime();
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

    const SpriteFrame *frame = player_get_frame(player);
    Rectangle rect = player_get_frame_rect(player);
    rect.x += player->position.x;
    rect.y += player->position.y;
    return rect;
}

Vector2 player_get_center(Player *player)
{
    const SpriteFrame *spriteFrame = &player->sprite->frames[player->spriteFrameIdx];
    Vector2 center = { 0 };
    center.y = player->position.y + spriteFrame->height / 2.0f;
    center.x = player->position.x + spriteFrame->width / 2.0f;
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
            player->state = PlayerState_SouthEast;
        } else if (offset.y < 0.0f) {
            player->state = PlayerState_NorthEast;
        } else {
            player->state = PlayerState_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            player->state = PlayerState_SouthWest;
        } else if (offset.y < 0.0f) {
            player->state = PlayerState_NorthWest;
        } else {
            player->state = PlayerState_West;
        }
    } else {
        if (offset.y > 0.0f) {
            player->state = PlayerState_South;
        } else if (offset.y < 0.0f) {
            player->state = PlayerState_North;
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

void player_update(Player *player)
{
    const double timeSinceLastMove = GetTime() - player->lastMoveTime;
    if (timeSinceLastMove > 10.0) {
        player->state = PlayerState_Idle;
    }

    // TODO: Set player->sprite to the correct sprite more intelligently?
    player->spriteFrameIdx = (size_t)player->state;
}

void player_draw(Player *player)
{
#if 0
    // Funny bug where texture stays still relative to screen, could be fun to abuse later
    const Rectangle charlieRec = player_get_rect(player);
#endif
    const Rectangle charlieRec = player_get_frame_rect(player);
    DrawTextureRec(*player->sprite->spritesheet->texture, charlieRec, player->position, WHITE);
}