#include "npc.h"
#include "draw_command.h"
#include "healthbar.h"
#include "helpers.h"
#include "maths.h"
#include "shadow.h"
#include "spritesheet.h"
#include "dlb_rand.h"
#include <cassert>
#include "particles.h"

void NPC::SetName(const char *newName, uint32_t newNameLen)
{
    memset(name, 0, nameLength);
    nameLength = MIN(newNameLen, USERNAME_LENGTH_MAX);
    memcpy(name, newName, nameLength);
    if (nameLength < USERNAME_LENGTH_MAX) {
        snprintf(name + nameLength, USERNAME_LENGTH_MAX - nameLength, " %d", id);
    }
}

Vector3 NPC::WorldCenter(void) const
{
    const Vector3 worldC = v3_add(body.WorldPosition(), sprite_center(sprite));
    return worldC;
}

Vector3 NPC::WorldTopCenter(void) const
{
    const Vector3 worldTopC = v3_add(body.WorldPosition(), sprite_top_center(sprite));
    return worldTopC;
}

float NPC::TakeDamage(float damage)
{
    return combat.TakeDamage(damage);
}

void NPC::UpdateDirection(Vector2 offset)
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

void NPC::Update(World &world, double dt)
{
    switch (type) {
        case Type::Type_Slime: Slime::Update(*this, world, dt); break;
    }

    body.Update(dt);
    sprite_update(sprite, dt);
    combat.Update(dt);

    if (g_clock.server && combat.diedAt && !combat.droppedDeathLoot) {
#if _DEBUG
        // If this fires, we're probably doing multiple interp/extrap for NPCs now, and need a flag similar
        // to input.skipFx in Player::Update() to prevent loot from rolling many times client-side.
        thread_local double lastRan = g_clock.now;
        DLB_ASSERT(g_clock.now > lastRan);
        lastRan = g_clock.now;
#endif
        world.lootSystem.SV_RollDrops(combat.lootTableId, [&](ItemStack dropStack) {
            world.itemSystem.SpawnItem(WorldCenter(), dropStack.uid, dropStack.count);
        });

        combat.droppedDeathLoot = true;
    }
}

float NPC::Depth(void) const
{
    return body.GroundPosition().y;
}

bool NPC::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void NPC::Draw(World &world)
{
    const Vector3 worldPos = body.WorldPosition();
    if (combat.hitPoints) {
        // Shadow size based on height from ground
        // https://yal.cc/top-down-bouncing-loot-effects/
        Shadow::Draw(worldPos, 16.0f * sprite.scale, -8.0f * sprite.scale);

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
        //const float radius = 5.0f;
        //DrawRectangleRec({ slimeBC.x - radius, slimeBC.y - radius, radius * 2, radius * 2 }, RED);
        body.Teleport({ worldPos.x, worldPos.y, 0 });
        sprite_draw_body(sprite, body, Fade(DARKGREEN, 0.7f));
    }
}