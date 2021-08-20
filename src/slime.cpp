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
    m_name = name;
    m_action = Action::None;
    m_body.restitution = 0.0f;
    m_body.drag = 0.95f;
    m_body.friction = 0.95f;
    m_body.lastUpdated = GetTime();
    m_sprite.spriteDef = &spriteDef;
    m_sprite.scale = 1.0f;
    m_sprite.direction = Direction::South;
    m_combat.maxHitPoints = 5.0f;
    m_combat.hitPoints = m_combat.maxHitPoints;
    m_randJumpIdle = 0.0;
}

void Slime::UpdateDirection(Vector2 offset)
{
    Direction prevDirection = m_sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            m_sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::East : Direction::South;
        } else if (offset.y < 0.0f) {
            m_sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::East : Direction::North;
        } else {
            m_sprite.direction = Direction::East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            m_sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::West : Direction::South;
        } else if (offset.y < 0.0f) {
            m_sprite.direction = fabs(offset.x) > fabs(offset.y) ? Direction::West : Direction::North;
        } else {
            m_sprite.direction = Direction::West;
        }
    } else {
        if (offset.y > 0.0f) {
            m_sprite.direction = Direction::South;
        } else if (offset.y < 0.0f) {
            m_sprite.direction = Direction::North;
        }
    }
    if (m_sprite.direction != prevDirection) {
        m_sprite.animFrameIdx = 0;
    }
}

bool Slime::Move(double now, double dt, Vector2 offset)
{
    UNUSED(dt);  // todo: use dt

    if (v2_is_zero(offset)) {
        return false;
    }

    // On ground and hasn't moved for a bit
    const double timeSinceLastMoved = now - m_body.lastMoved;
    if (m_body.position.z == 0.0f && timeSinceLastMoved > m_randJumpIdle) {
        m_body.velocity.x += offset.x;
        m_body.velocity.y += offset.y;
        m_body.velocity.z += METERS_TO_PIXELS(3.0f);
        m_randJumpIdle = (double)dlb_rand32f_range(1.0f, 2.5f);
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
    if (this->m_sprite.scale > other.m_sprite.scale) {
        a = this;
        b = &other;
    } else {
        a = &other;
        b = this;
    }

    // Limit max scale
    float newScale = a->m_sprite.scale + 0.5f * b->m_sprite.scale;
    if (newScale > SLIME_MAX_SCALE) {
        return false;
    }

    // Combine slime B's attributes into slime A
    a->m_sprite.scale        = newScale;
    a->m_combat.hitPoints    = a->m_combat.hitPoints    + 0.5f * b->m_combat.hitPoints;
    a->m_combat.maxHitPoints = a->m_combat.maxHitPoints + 0.5f * b->m_combat.maxHitPoints;
    //Vector3 halfAToB = v3_scale(v3_sub(b->body.position, a->body.position), 0.5f);
    //a->body.position = v3_add(a->body.position, halfAToB);

    // Kill slime B
    b->m_combat.hitPoints = 0.0f;
    return true;
}

bool Slime::Attack(double now, double dt)
{
    UNUSED(dt); // todo: use dt;

    if (m_action == Action::None) {
        m_action = Action::Attack;
        m_combat.attackStartedAt = now;
        m_combat.attackDuration = 0.1;
        return true;
    }
    return false;
}

void Slime::Update(double now, double dt)
{
    const double timeSinceAttackStarted = now - m_combat.attackStartedAt;
    if (timeSinceAttackStarted > m_combat.attackDuration) {
        m_action = Action::None;
        m_combat.attackStartedAt = 0;
        m_combat.attackDuration = 0;
    }

    m_body.Update(now, dt);
    sprite_update(m_sprite, now, dt);
}

float Slime::Depth() const
{
    return m_body.position.y;
}

bool Slime::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(m_sprite, m_body, cullRect);
    return cull;
}

void Slime::Push() const
{
    draw_command_push(DrawableType::Slime, this);
}

void Slime::Draw() const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime.transform.position.z / 20.0f;
    const Vector2 slimeBC = m_body.GroundPosition();
    Shadow::Draw((int)slimeBC.x, (int)slimeBC.y, 16.0f * m_sprite.scale, -8.0f * m_sprite.scale);

    sprite_draw_body(m_sprite, m_body, Fade(WHITE, 0.7f));
    HealthBar::Draw(10, m_sprite, m_body, m_combat.hitPoints, m_combat.maxHitPoints);
}