#include "slime.h"
#include "spritesheet.h"
#include "vector2f.h"
#include <assert.h>

void slime_init(Slime *slime, const char *name, struct Sprite *sprite)
{
    assert(slime);
    //assert(name);
    assert(sprite);

    slime->name = name;
    slime->facing = SlimeFacing_South;
    slime->maxHitPoints = 10.0f;
    slime->hitPoints = slime->maxHitPoints;
    slime->sprite = sprite;
}

SpriteFrame *slime_get_frame(const Slime *slime)
{
    SpriteFrame *frame = &slime->sprite->frames[slime->spriteFrameIdx];
    return frame;
}

Rectangle slime_get_frame_rect(const Slime *slime)
{
    assert(slime->spriteFrameIdx < slime->sprite->frameCount);

    const SpriteFrame *frame = slime_get_frame(slime);
    Rectangle rect = { 0 };
    rect.x = (float)frame->x;
    rect.y = (float)frame->y;
    rect.width = (float)frame->width;
    rect.height = (float)frame->height;
    return rect;
}

Rectangle slime_get_rect(const Slime *slime)
{
    assert(slime->spriteFrameIdx < slime->sprite->frameCount);

    Rectangle frameRect = slime_get_frame_rect(slime);
    Rectangle rect = { 0 };
    rect.x = slime->position.x;
    rect.y = slime->position.y;
    rect.width = frameRect.width;
    rect.height = frameRect.height;
    return rect;
}

Vector2 slime_get_center(const Slime *slime)
{
    const SpriteFrame *spriteFrame = &slime->sprite->frames[slime->spriteFrameIdx];
    Vector2 center = { 0 };
    center.x = slime->position.x + spriteFrame->width / 2.0f;
    center.y = slime->position.y + spriteFrame->height / 2.0f;
    return center;
}

Vector2 slime_get_bottom_center(const Slime *slime)
{
    const SpriteFrame *spriteFrame = &slime->sprite->frames[slime->spriteFrameIdx];
    Vector2 center = { 0 };
    center.x = slime->position.x + spriteFrame->width / 2.0f;
    center.y = slime->position.y + spriteFrame->height;
    return center;
}

static void update_direction(Slime *slime, Vector2 offset)
{
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            slime->facing = fabs(offset.x) > fabs(offset.y) ? SlimeFacing_East : SlimeFacing_South;
        } else if (offset.y < 0.0f) {
            slime->facing = fabs(offset.x) > fabs(offset.y) ? SlimeFacing_East : SlimeFacing_North;
        } else {
            slime->facing = SlimeFacing_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            slime->facing = fabs(offset.x) > fabs(offset.y) ? SlimeFacing_West : SlimeFacing_South;
        } else if (offset.y < 0.0f) {
            slime->facing = fabs(offset.x) > fabs(offset.y) ? SlimeFacing_West : SlimeFacing_North;
        } else {
            slime->facing = SlimeFacing_West;
        }
    } else {
        if (offset.y > 0.0f) {
            slime->facing = SlimeFacing_South;
        } else if (offset.y < 0.0f) {
            slime->facing = SlimeFacing_North;
        }
    }
#undef ALTERNATE_DIAG
}

void slime_move(Slime *slime, Vector2 offset)
{
    assert(slime);
    if (v2_is_zero(offset)) return;

    slime->position = v2_add(slime->position, offset);
    update_direction(slime, offset);
}

bool slime_attack(Slime *slime)
{
    if (slime->action == SlimeAction_None) {
        slime->action = SlimeAction_Attack;
        slime->attackStartedAt = GetTime();
        slime->attackDuration = 0.1;
        return true;
    }
    return false;
}

void slime_update(Slime *slime)
{
    const double t = GetTime();

    const double timeSinceAttackStarted = t - slime->attackStartedAt;
    if (timeSinceAttackStarted > slime->attackDuration) {
        slime->action = SlimeAction_None;
        slime->attackStartedAt = 0;
        slime->attackDuration = 0;
    }

    // TODO: Set slime->sprite to the correct sprite more intelligently?
    // TODO: Could use bitmask with & on the various states / flags
    int frameIdx = slime->facing;
    frameIdx += slime->action * SlimeFacing_Count;

    slime->spriteFrameIdx = (size_t)frameIdx;
}

void slime_draw(Slime *slime)
{
#if 0
    // Funny bug where texture stays still relative to screen, could be fun to abuse later
    const Rectangle rect = slime_get_rect(slime);
#endif
    const Rectangle rect = slime_get_frame_rect(slime);
    DrawTextureRec(*slime->sprite->spritesheet->texture, rect, slime->position, WHITE);
}