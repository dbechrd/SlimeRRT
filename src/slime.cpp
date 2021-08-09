#include "slime.h"
#include "draw_command.h"
#include "healthbar.h"
#include "helpers.h"
#include "maths.h"
#include "shadow.h"
#include "spritesheet.h"
#include "dlb_rand.h"
#include <cassert>
#include "particles.h"

#define SLIME_MAX_SCALE 3.0f

Slime::Slime(const char *name, const SpriteDef &spriteDef) : Slime()
{
    name = name;
    body.restitution = 0.0f;
    body.drag = 0.95f;
    body.friction = 0.95f;
    body.lastUpdated = GetTime();
    sprite.spriteDef = &spriteDef;
    sprite.scale = 1.0f;
    sprite.direction = Direction_South;
    combat.maxHitPoints = 5.0f;
    combat.hitPoints = combat.maxHitPoints;
}

void Slime::UpdateDirection(Vector2 offset)
{
    Direction prevDirection = sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_East : Direction_South;
        } else if (offset.y < 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_East : Direction_North;
        } else {
            sprite.direction = Direction_East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_West : Direction_South;
        } else if (offset.y < 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction_West : Direction_North;
        } else {
            sprite.direction = Direction_West;
        }
    } else {
        if (offset.y > 0.0f) {
            sprite.direction = Direction_South;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction_North;
        }
    }
    if (sprite.direction != prevDirection) {
        sprite.animFrameIdx = 0;
    }
}

bool Slime::Move(double now, double dt, Vector2 offset)
{
    UNUSED(dt);  // todo: use dt

    if (v2_is_zero(offset)) {
        return false;
    }

    // On ground and hasn't moved for a bit
    const double timeSinceLastMoved = now - body.lastMoved;
    if (body.position.z == 0.0f && timeSinceLastMoved > randJumpIdle) {
        body.velocity.x += offset.x;
        body.velocity.y += offset.y;
        body.velocity.z += METERS_TO_PIXELS(3.0f);
        randJumpIdle = (double)dlb_rand32f_range(1.0f, 2.5f);
        UpdateDirection(offset);
        return true;
    }
    return false;
}

bool Slime::Combine(Slime &other)
{
    // The bigger slime should absorb the smaller one
    Slime *a = nullptr;
    Slime *b = nullptr;
    if (a->sprite.scale > b->sprite.scale) {
        a = this;
        b = &other;
    } else {
        a = &other;
        b = this;
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

bool Slime::Attack(double now, double dt)
{
    UNUSED(dt); // todo: use dt;

    if (action == Action_None) {
        action = Action_Attack;
        combat.attackStartedAt = now;
        combat.attackDuration = 0.1;
        return true;
    }
    return false;
}

void Slime::Update(double now, double dt)
{
    const double timeSinceAttackStarted = now - combat.attackStartedAt;
    if (timeSinceAttackStarted > combat.attackDuration) {
        action = Action_None;
        combat.attackStartedAt = 0;
        combat.attackDuration = 0;
    }

    body_update(&body, now, dt);
    sprite_update(sprite, now, dt);
}

float Slime::Depth() const
{
    return body.position.y;
}

bool Slime::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void Slime::Push() const
{
    draw_command_push(DrawableType_Slime, this);
}

void Slime::Draw() const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime.transform.position.z / 20.0f;
    const Vector2 slimeBC = body_ground_position(&body);
    shadow_draw((int)slimeBC.x, (int)slimeBC.y, 16.0f * sprite.scale, -8.0f * sprite.scale);

    sprite_draw_body(sprite, body, Fade(WHITE, 0.7f));
    healthbar_draw(10, sprite, body, combat.hitPoints, combat.maxHitPoints);
}