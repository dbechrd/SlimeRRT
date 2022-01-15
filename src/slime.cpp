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

#define SLIME_MAX_SCALE 3

void Slime::Init(void)
{
    assert(!sprite.spriteDef);

    body.restitution = 0.0f;
    body.drag = 0.95f;
    body.friction = 0.95f;

    sprite.scale = 1;
    const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Slime);
    const SpriteDef *spriteDef = spritesheet.FindSprite("slime");
    if (spriteDef) {
        sprite.spriteDef = spriteDef;
    }

    combat.hitPointsMax = 30;
    combat.hitPoints = combat.hitPointsMax;
    combat.meleeDamage = 3;
    combat.lootTableId = LootTableID::LT_Slime;
    randJumpIdle = 0.0;
}

void Slime::SetName(const char *slimeName, uint32_t slimeNameLength)
{
    nameLength = MIN(slimeNameLength, USERNAME_LENGTH_MAX);
    memcpy(name, slimeName, nameLength);
}

void Slime::UpdateDirection(Vector3i &offset)
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

bool Slime::Move(double dt, Vector3i offset)
{
    UNUSED(dt);  // todo: use dt

    if (!body.OnGround()) {
        return false;
    }

    moveState = MoveState::Idle;
    if (v3_is_zero(offset)) {
        return false;
    }

    // On ground and hasn't moved for a bit
    const double timeSinceJump = glfwGetTime() - body.lastMoved;
    if (timeSinceJump > randJumpIdle) {
        moveState = MoveState::Jump;
        body.velocity.x += offset.x;
        body.velocity.y += offset.y;
        body.velocity.z += METERS_TO_PIXELS(3);
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
    int newScale = a->sprite.scale + (b->sprite.scale / 2);
    if (newScale > SLIME_MAX_SCALE) {
        return false;
    }

    // Combine slime B's attributes into slime A
    a->sprite.scale        = newScale;
    a->combat.hitPoints    = a->combat.hitPoints    + (b->combat.hitPoints / 2);
    a->combat.hitPointsMax = a->combat.hitPointsMax + (b->combat.hitPointsMax / 2);
    //Vector3 halfAToB = v3_scale(v3_sub(b->body.position, a->body.position), 0.5f);
    //a->body.position = v3_add(a->body.position, halfAToB);

    // Kill slime B
    b->combat.hitPoints = 0;
    return true;
}

bool Slime::Attack(double dt)
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
        actionState = ActionState::Attacking;
        body.lastMoved = glfwGetTime();
        combat.attackStartedAt = glfwGetTime();
        combat.attackDuration = 0.0;
        return true;
    }
#endif
    return false;
}

void Slime::Update(double dt)
{
    switch (actionState) {
        case ActionState::Attacking: {
            const double timeSinceAttackStarted = glfwGetTime() - combat.attackStartedAt;
            if (timeSinceAttackStarted > combat.attackDuration) {
                actionState = ActionState::None;
                combat.attackStartedAt = 0;
                combat.attackDuration = 0;
                combat.attackFrame = 0;
            } else {
                combat.attackFrame++;
            }
            break;
        }
    }

    body.Update(dt);
    sprite_update(sprite, dt);
}

int Slime::Depth(void) const
{
    return body.position.y;
}

bool Slime::Cull(const Recti &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void Slime::Draw(void) const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + transform.position.z / 20.0f;
    const Vector3i slimeBC = body.GroundPosition();
    Shadow::Draw(slimeBC.x, slimeBC.y, 16 * sprite.scale, -8 * sprite.scale);

    sprite_draw_body(sprite, body, Fade(WHITE, 0.7f));
    HealthBar::Draw(10, sprite, body, name, combat.hitPoints, combat.hitPointsMax);
}