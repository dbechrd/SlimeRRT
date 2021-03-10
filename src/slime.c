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
    slime->body.scale = 1.0f;
    slime->body.facing = Facing_South;
    slime->body.alpha = 0.7f;
    slime->body.sprite = sprite;
    slime->combat.maxHitPoints = 10.0f;
    slime->combat.hitPoints = slime->combat.maxHitPoints;
}

static void update_direction(Slime *slime, Vector2 offset)
{
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            slime->body.facing = fabs(offset.x) > fabs(offset.y) ? Facing_East : Facing_South;
        } else if (offset.y < 0.0f) {
            slime->body.facing = fabs(offset.x) > fabs(offset.y) ? Facing_East : Facing_North;
        } else {
            slime->body.facing = Facing_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            slime->body.facing = fabs(offset.x) > fabs(offset.y) ? Facing_West : Facing_South;
        } else if (offset.y < 0.0f) {
            slime->body.facing = fabs(offset.x) > fabs(offset.y) ? Facing_West : Facing_North;
        } else {
            slime->body.facing = Facing_West;
        }
    } else {
        if (offset.y > 0.0f) {
            slime->body.facing = Facing_South;
        } else if (offset.y < 0.0f) {
            slime->body.facing = Facing_North;
        }
    }
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
    slimeA->combat.hitPoints    += 0.5f * slimeB->combat.hitPoints;
    slimeA->combat.maxHitPoints += 0.5f * slimeB->combat.maxHitPoints;
    slimeA->body.scale          += 0.5f * slimeB->body.scale;
    Vector3 halfAToB = v3_scale(v3_sub(slimeB->body.position, slimeA->body.position), 0.5f);
    slimeA->body.position = v3_add(slimeA->body.position, halfAToB);

    // Kill slime B
    slimeB->combat.hitPoints = 0.0f;
}

bool slime_attack(Slime *slime)
{
    if (slime->action == SlimeAction_None) {
        slime->action = SlimeAction_Attack;
        slime->combat.attackStartedAt = GetTime();
        slime->combat.attackDuration = 0.1;
        return true;
    }
    return false;
}

void slime_update(Slime *slime)
{
    const double t = GetTime();

    const double timeSinceAttackStarted = t - slime->combat.attackStartedAt;
    if (timeSinceAttackStarted > slime->combat.attackDuration) {
        slime->action = SlimeAction_None;
        slime->combat.attackStartedAt = 0;
        slime->combat.attackDuration = 0;
    }

    // TODO: Set slime->sprite to the correct sprite more intelligently?
    // TODO: Could use bitmask with & on the various states / flags
    int frameIdx = slime->body.facing;
    frameIdx += slime->action * Facing_Count;

    slime->body.spriteFrameIdx = (size_t)frameIdx;
    assert(slime->body.spriteFrameIdx < slime->body.sprite->frameCount);
}

void slime_draw(Slime *slime)
{
    body_draw(&slime->body);
}