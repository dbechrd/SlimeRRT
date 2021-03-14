#include "slime.h"
#include "spritesheet.h"
#include "math.h"
#include "helpers.h"
#include <assert.h>

#define SLIME_MAX_SCALE 3.0f

void slime_init(Slime *slime, const char *name, struct Sprite *sprite)
{
    assert(slime);
    //assert(name);
    assert(sprite);

    slime->name = name;
    slime->body.scale = 1.0f;
    slime->body.restitution = 0.0f;
    slime->body.drag = 0.95f;
    slime->body.friction = 0.95f;
    slime->body.lastUpdated = GetTime();
    slime->body.facing = Facing_South;
    slime->body.color = WHITE;
    slime->body.color.a = (unsigned char)(255.0f * 0.7f);
    slime->body.sprite = sprite;
    slime->combat.maxHitPoints = 10.0f;
    slime->combat.hitPoints = slime->combat.maxHitPoints;
}

static void update_direction(Slime *slime, Vector2 offset)
{
    Facing prevFacing = slime->body.facing;
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
    if (slime->body.facing != prevFacing) {
        slime->body.animFrameIdx = 0;
    }
}

void slime_move(Slime *slime, double now, double dt, Vector2 offset)
{
    assert(slime);
    if (v2_is_zero(offset)) return;

    // On ground and hasn't moved for a bit
    const double timeSinceLastMoved = now - slime->body.lastMoved;
    if (slime->body.position.z == 0.0f && timeSinceLastMoved > slime->randJumpIdle) {
        slime->body.velocity.x += offset.x;
        slime->body.velocity.y += offset.y;
        slime->body.velocity.z += METERS(3.0f);
        slime->randJumpIdle = 0.5f + random_normalized(50) * 1.5f;
    }
    update_direction(slime, offset);
}

void slime_combine(Slime *slimeA, Slime *slimeB)
{
    // Limit max scale
    if (slimeA->body.scale >= SLIME_MAX_SCALE && slimeB->body.scale >= SLIME_MAX_SCALE) {
        return;
    }

    // Combine slime B's attributes into Slime A
    slimeA->combat.hitPoints    += 0.5f * slimeB->combat.hitPoints;
    slimeA->combat.maxHitPoints += 0.5f * slimeB->combat.maxHitPoints;
    slimeA->body.scale          += 0.5f * slimeB->body.scale;
    Vector3 halfAToB = v3_scale(v3_sub(slimeB->body.position, slimeA->body.position), 0.5f);
    slimeA->body.position = v3_add(slimeA->body.position, halfAToB);

    // Kill slime B
    slimeB->combat.hitPoints = 0.0f;
}

bool slime_attack(Slime *slime, double now, double dt)
{
    if (slime->action == SlimeAction_None) {
        slime->action = SlimeAction_Attack;
        slime->combat.attackStartedAt = now;
        slime->combat.attackDuration = 0.1;
        return true;
    }
    return false;
}

void slime_update(Slime *slime, double now, double dt)
{
    const double timeSinceAttackStarted = now - slime->combat.attackStartedAt;
    if (timeSinceAttackStarted > slime->combat.attackDuration) {
        slime->action = SlimeAction_None;
        slime->combat.attackStartedAt = 0;
        slime->combat.attackDuration = 0;
    }

    body_update(&slime->body, now, dt);
}

void slime_draw(Slime *slime)
{
    body_draw(&slime->body);
}