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

void Slime::Init(void)
{
    assert(!sprite.spriteDef);

    SetName(CSTR("Slime"));

    body.speed = SV_SLIME_MOVE_SPEED;
    body.drag = 0.95f;
    body.friction = 0.95f;

    combat.level = 1;
    combat.hitPointsMax = 10.0f;
    combat.hitPoints = combat.hitPointsMax;
    combat.meleeDamage = 10.0f;
    combat.xpMin = 2;
    combat.xpMax = 4;
    combat.lootTableId = LootTableID::LT_Slime;
    //combat.droppedHitLoot = false;
    //combat.droppedDeathLoot = false;

    sprite.scale = 1.0f;
    const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Slime);
    const SpriteDef *spriteDef = spritesheet.FindSprite("slime");
    if (spriteDef) {
        sprite.spriteDef = spriteDef;
    }

    //randJumpIdle = 0.0;
}

void Slime::SetName(const char *slimeName, uint32_t slimeNameLength)
{
    memset(name, 0, nameLength);
    nameLength = MIN(slimeNameLength, USERNAME_LENGTH_MAX);
    memcpy(name, slimeName, nameLength);
    if (nameLength < USERNAME_LENGTH_MAX) {
        snprintf(name + nameLength, USERNAME_LENGTH_MAX - nameLength, " %d", id);
    }
}

Vector3 Slime::WorldCenter(void) const
{
    const Vector3 worldC = v3_add(body.WorldPosition(), sprite_center(sprite));
    return worldC;
}

Vector3 Slime::WorldTopCenter(void) const
{
    const Vector3 worldTopC = v3_add(body.WorldPosition(), sprite_top_center(sprite));
    return worldTopC;
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

bool Slime::Move(double dt, Vector2 offset)
{
    UNUSED(dt);  // todo: use dt

    // Slime must be on ground to move
    if (!body.OnGround()) {
        return false;
    }

    moveState = MoveState::Idle;
    if (v2_is_zero(offset)) {
        return false;
    }

    // Check if hasn't moved for a bit
    if (body.TimeSinceLastMove() > randJumpIdle) {
        moveState = MoveState::Jump;
        body.velocity.x += offset.x;
        body.velocity.y += offset.y;
        body.velocity.z += METERS_TO_PIXELS(4.0f);
        randJumpIdle = (double)dlb_rand32f_range(0.5f, 1.5f) / sprite.scale;
        UpdateDirection(offset);
        return true;
    }
    return false;
}

bool Slime::TryCombine(Slime &other)
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
    a->combat.hitPointsMax = a->combat.hitPointsMax + 0.5f * b->combat.hitPointsMax;
    //Vector3 halfAToB = v3_scale(v3_sub(b->body.position, a->body.position), 0.5f);
    //a->body.position = v3_add(a->body.position, halfAToB);

    // Kill slime B
    b->combat.hitPoints = 0.0f;
    b->combat.diedAt = g_clock.now;
    b->combat.droppedDeathLoot = true;
#if SV_DEBUG_WORLD_ENEMIES
    TraceLog(LOG_DEBUG, "Combined slime #%u into slime #%u", b->type, a->type);
#endif
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
    if (body.JustLanded()) {
        actionState = ActionState::Attacking;
        body.Move({});  // update last move to stop idle animation
        combat.attackStartedAt = g_clock.now;
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
            const double timeSinceAttackStarted = g_clock.now - combat.attackStartedAt;
            if (timeSinceAttackStarted > combat.attackDuration) {
                actionState = ActionState::None;
                combat.attackStartedAt = 0;
                combat.attackDuration = 0;
            }
            break;
        }
    }

    body.Update(dt);
    sprite_update(sprite, dt);
    combat.Update(dt);
}

float Slime::Depth(void) const
{
    return body.GroundPosition().y;
}

bool Slime::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void Slime::Draw(World &world) const
{
    const Vector2 slimeBC = body.GroundPosition();
    if (combat.hitPoints) {
        // TODO: Shadow size based on height from ground
        // https://yal.cc/top-down-bouncing-loot-effects/
        //const float shadowScale = 1.0f + transform.position.z / 20.0f;
        Shadow::Draw((int)slimeBC.x, (int)slimeBC.y, 16.0f * sprite.scale, -8.0f * sprite.scale);

        sprite_draw_body(sprite, body, Fade(WHITE, 0.7f));
        Vector3 topCenter = WorldTopCenter();
        HealthBar::Draw({ topCenter.x, topCenter.y - topCenter.z }, name, combat);

#if CL_DEMO_SNAPSHOT_RADII
        // DEBUG: Draw stale visual marker if no snapshot received in a while
        if (body.positionHistory.Count()) {
            auto &lastPos = body.positionHistory.Last();
            if (g_clock.now - lastPos.recvAt > 3.0f) {
                DrawCircleV(body.VisualPosition(), 3.0f, BEIGE);
            }
        }
#endif
    } else {
        const float radius = 5.0f;
        DrawRectangleRec({ slimeBC.x - radius, slimeBC.y - radius, radius * 2, radius * 2 }, RED);
    }
}