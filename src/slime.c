#include "slime.h"
#include "draw_command.h"
#include "spritesheet.h"
#include "helpers.h"
#include "dlb_rand.h"
#include "maths.h"
#include <assert.h>
#include "particles.h"

#define SLIME_MAX_SCALE 3.0f

void slime_init(Slime *slime, const char *name, const SpriteDef *spriteDef)
{
    assert(slime);
    //assert(name);
    assert(spriteDef);

    slime->name = name;
    slime->body.restitution = 0.0f;
    slime->body.drag = 0.95f;
    slime->body.friction = 0.95f;
    slime->body.lastUpdated = GetTime();
    slime->sprite.spriteDef = spriteDef;
    slime->sprite.scale = 1.0f;
    slime->sprite.direction = Direction_South;
    slime->combat.maxHitPoints = 5.0f;
    slime->combat.hitPoints = slime->combat.maxHitPoints;
}

static void update_direction(Slime *slime, Vector2 offset)
{
    assert(slime);

    Direction prevDirection = slime->sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            slime->sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_East : Direction_South;
        } else if (offset.y < 0.0f) {
            slime->sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_East : Direction_North;
        } else {
            slime->sprite.direction = Direction_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            slime->sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_West : Direction_South;
        } else if (offset.y < 0.0f) {
            slime->sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_West : Direction_North;
        } else {
            slime->sprite.direction = Direction_West;
        }
    } else {
        if (offset.y > 0.0f) {
            slime->sprite.direction = Direction_South;
        } else if (offset.y < 0.0f) {
            slime->sprite.direction = Direction_North;
        }
    }
    if (slime->sprite.direction != prevDirection) {
        slime->sprite.animFrameIdx = 0;
    }
}

bool slime_move(Slime *slime, double now, double dt, Vector2 offset)
{
    assert(slime);
    UNUSED(dt);  // todo: use dt

    if (v2_is_zero(offset)) {
        return false;
    }

    // On ground and hasn't moved for a bit
    const double timeSinceLastMoved = now - slime->body.lastMoved;
    if (slime->body.position.z == 0.0f && timeSinceLastMoved > slime->randJumpIdle) {
        slime->body.velocity.x += offset.x;
        slime->body.velocity.y += offset.y;
        slime->body.velocity.z += METERS_TO_PIXELS(3.0f);
        slime->randJumpIdle = (double)dlb_rand32f_range(1.0f, 2.5f);
        update_direction(slime, offset);
        return true;
    }
    return false;
}

bool slime_combine(Slime *slimeA, Slime *slimeB)
{
    assert(slimeA);
    assert(slimeB);

    // The bigger slime should absorb the smaller one
    Slime *a = slimeA;
    Slime *b = slimeB;
    if (slimeB->sprite.scale > slimeA->sprite.scale) {
        Slime *tmp = a;
        a = b;
        b = tmp;
    }

    // Limit max scale
    float newScale = a->sprite.scale + 0.5f * b->sprite.scale;
    if (newScale > SLIME_MAX_SCALE) {
        return false;
    }

    // Combine slime B's attributes into slime A
    a->sprite.scale        = newScale;
    a->combat.hitPoints    = a->combat.hitPoints    + 0.5f * b->combat.hitPoints;
    a->combat.maxHitPoints = a->combat.maxHitPoints + 0.5f * b->combat.maxHitPoints;
    //Vector3 halfAToB = v3_scale(v3_sub(b->body.position, a->body.position), 0.5f);
    //a->body.position = v3_add(a->body.position, halfAToB);

    // Kill slime B
    b->combat.hitPoints = 0.0f;
    return true;
}

bool slime_attack(Slime *slime, double now, double dt)
{
    assert(slime);
    UNUSED(dt); // todo: use dt;

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
    assert(slime);

    const double timeSinceAttackStarted = now - slime->combat.attackStartedAt;
    if (timeSinceAttackStarted > slime->combat.attackDuration) {
        slime->action = SlimeAction_None;
        slime->combat.attackStartedAt = 0;
        slime->combat.attackDuration = 0;
    }

    body_update(&slime->body, now, dt);
    sprite_update(&slime->sprite, now, dt);
}

float slime_depth(const Slime *slime)
{
    const float depth = slime->body.position.y;
    return depth;
}

void slime_push(const Slime *slime)
{
    draw_command_push(DrawableType_Slime, slime);
}

void slime_draw(const Slime *slime)
{
    assert(slime);

    sprite_draw_body(&slime->sprite, &slime->body, slime->sprite.scale, Fade(WHITE, 0.7f));
}