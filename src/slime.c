#include "slime.h"
#include "spritesheet.h"
#include "math.h"
#include <assert.h>

void slime_init(Slime *slime, const char *name, struct Sprite *sprite)
{
    assert(slime);
    //assert(name);
    assert(sprite);

    slime->name = name;
    slime->scale = 1.0f;
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
    rect.x = slime->body.position.x - frameRect.width / 2.0f * slime->scale;
    rect.y = slime->body.position.y - frameRect.height * slime->scale - slime->body.position.z;
    rect.width = frameRect.width * slime->scale;
    rect.height = frameRect.height * slime->scale;
    return rect;
}

Vector3 slime_get_center(const Slime *slime)
{
    const SpriteFrame *spriteFrame = &slime->sprite->frames[slime->spriteFrameIdx];
    Vector3 center = { 0 };
    center.x = slime->body.position.x;
    center.y = slime->body.position.y;
    center.z = slime->body.position.z + spriteFrame->height / 2.0f * slime->scale;
    return center;
}

Vector2 slime_get_ground_position(const Slime *slime)
{
    Vector2 groundPosition = { 0 };
    groundPosition.x = slime->body.position.x;
    groundPosition.y = slime->body.position.y;
    return groundPosition;
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

    slime->body.position.x += offset.x;
    slime->body.position.y += offset.y;
    update_direction(slime, offset);
}

void slime_combine(Slime *slimeA, Slime *slimeB)
{
    // Combine slime B's attributes into Slime A
    slimeA->hitPoints       += 0.5f * slimeB->hitPoints;
    slimeA->maxHitPoints    += 0.5f * slimeB->maxHitPoints;
    slimeA->scale           += 0.5f * slimeB->scale;
    Vector3 halfAToB = v3_scale(v3_sub(slimeB->body.position, slimeA->body.position), 0.5f);
    slimeA->body.position = v3_add(slimeA->body.position, halfAToB);

    // Kill slime B
    slimeB->hitPoints = 0.0f;
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
    //DrawTextureRec(*slime->sprite->spritesheet->texture, rect, slime->transform.position, WHITE);

    Rectangle dest = slime_get_rect(slime);

#if _DEBUG
    // DEBUG: Draw collision rectangle
    DrawRectangleRec(dest, Fade(RED, 0.2f));
#endif

    // Draw textured sprite
    DrawTextureTiled(*slime->sprite->spritesheet->texture, rect, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, slime->scale, Fade(WHITE, 0.7f));

#if _DEBUG
    // DEBUG: Draw bottom bottomCenter
    Vector2 groundPos = slime_get_ground_position(slime);
    DrawCircle((int)groundPos.x, (int)groundPos.y, 4.0f, RED);
#endif
}