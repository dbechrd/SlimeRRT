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

Slime::Slime(const char *slimeName, const SpriteDef *spriteDef)
{
    name = slimeName;
    action = Action::None;
    body.restitution = 0.0f;
    body.drag = 0.95f;
    body.friction = 0.95f;
    body.lastUpdated = GetTime();
    sprite.spriteDef = spriteDef;
    sprite.scale = 1.0f;
    sprite.direction = Direction::South;
    combat.maxHitPoints = 5.0f;
    combat.hitPoints = combat.maxHitPoints;
    combat.meleeDamage = 3.0f;
    combat.lootTableId = LootTableID::LT_Slime;
    randJumpIdle = 0.0;
}

void Slime::UpdateDirection(Vector2 offset)
{
    Direction prevDirection = sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::East : Direction::South;
        } else if (offset.y < 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::East : Direction::North;
        } else {
            sprite.direction = Direction::East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::West : Direction::South;
        } else if (offset.y < 0.0f) {
            sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::West : Direction::North;
        } else {
            sprite.direction = Direction::West;
        }
    } else {
        if (offset.y > 0.0f) {
            sprite.direction = Direction::South;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction::North;
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

    if (!body.OnGround()) {
        return false;
    }

    // On ground and hasn't moved for a bit
    const double timeSinceJump = now - body.lastMoved;
    if (timeSinceJump > randJumpIdle) {
        action = Action::Jump;
        body.velocity.x += offset.x;
        body.velocity.y += offset.y;
        body.velocity.z += METERS_TO_PIXELS(3.0f);
        randJumpIdle = (double)dlb_rand32f_range(1.0f, 2.5f) / sprite.scale;
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
    if (sprite.scale > other.sprite.scale) {
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

#if 0
    if (!body.landed) {
        return false;
    }

    const double timeSinceAttackStarted = now - combat.attackStartedAt;
    if (timeSinceAttackStarted > (combat.attackDuration / sprite.scale)) {
        action = Action::Attack;
        combat.attackStartedAt = now;
        combat.attackDuration = 0.1;
        return true;
    }
#else
    if (body.landed) {
        action = Action::Attack;
        combat.attackStartedAt = now;
        combat.attackDuration = 0.0;
        return true;
    }
#endif
    return false;
}

void Slime::Update(double now, double dt)
{
    switch (action) {
        case Action::Jump: {
            if (body.OnGround()) {
                action = Action::None;
            }
            break;
        } case Action::Attack: {
            const double timeSinceAttackStarted = now - combat.attackStartedAt;
            if (timeSinceAttackStarted > combat.attackDuration) {
                action = Action::None;
                combat.attackStartedAt = 0;
                combat.attackDuration = 0;
            }
            break;
        }
    }
    body.Update(now, dt);
    sprite_update(sprite, now, dt);
}

void Slime::Push(DrawList &drawList) const
{
    Drawable drawable{ Drawable_Slime };
    drawable.slime = this;
    drawList.Push(drawable);
}

float Slime_Depth(const Drawable &drawable)
{
    assert(drawable.type == Drawable_Slime);
    const Slime &slime = *drawable.slime;
    return slime.body.position.y;
}

bool Slime_Cull(const Drawable &drawable, const Rectangle &cullRect)
{
    assert(drawable.type == Drawable_Slime);
    const Slime &slime = *drawable.slime;
    bool cull = sprite_cull_body(slime.sprite, slime.body, cullRect);
    return cull;
}

void Slime_Draw(const Drawable &drawable)
{
    assert(drawable.type == Drawable_Slime);
    const Slime &slime = *drawable.slime;
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime.transform.position.z / 20.0f;
    const Vector2 slimeBC = slime.body.GroundPosition();
    Shadow::Draw((int)slimeBC.x, (int)slimeBC.y, 16.0f * slime.sprite.scale, -8.0f * slime.sprite.scale);

    sprite_draw_body(slime.sprite, slime.body, Fade(WHITE, 0.7f));
    HealthBar::Draw(10, slime.sprite, slime.body, slime.name, slime.combat.hitPoints, slime.combat.maxHitPoints);
}