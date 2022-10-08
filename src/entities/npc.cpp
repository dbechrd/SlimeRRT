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
    nameLength = MIN(newNameLen, sizeof(name));
    memcpy(name, newName, nameLength);
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
    DLB_ASSERT(!despawnedAt);

    // Update animations
    combat.Update(dt);
    sprite_update(sprite, dt);

    if (!combat.diedAt) {
        switch (type) {
            case Type::Type_Slime: Slime::Update(*this, world, dt); break;
        }
        body.Update(dt);
    } else if (!body.Resting()) {
        body.Update(dt);
    }

    if (g_clock.server && combat.diedAt && !combat.droppedDeathLoot) {
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
    UNUSED(world);
    DLB_ASSERT(!despawnedAt);

    const Vector3 groundPos = body.GroundPosition3();

    // Usually 1.0, fades to zero after death
    const float sinceDeath = combat.diedAt ? (float)((g_clock.now - combat.diedAt) / CL_NPC_CORPSE_LIFETIME) : 0;
    const float deadAlpha = CLAMP(1.0f - sinceDeath, 0, 1);

    // Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    Shadow::Draw(groundPos, 16.0f * sprite.scale, -8.0f * sprite.scale, deadAlpha);
    sprite_draw_body(sprite, body, Fade(WHITE, 0.7f * deadAlpha));

    if (!combat.diedAt) {
        Vector3 topCenter = WorldTopCenter();
        HealthBar::Draw({ topCenter.x, topCenter.y - topCenter.z }, name, combat, id);
    }

#if CL_DEMO_SNAPSHOT_RADII
    // DEBUG: Draw stale visual marker if no snapshot received in a while
    if (body.positionHistory.Count()) {
        auto &lastPos = body.positionHistory.Last();
        if (g_clock.now - lastPos.serverTime > CL_NPC_STALE_LIFETIME) {
            Vector2 vizPos = body.VisualPosition();
            const int radius = 7;
            DrawRectangle((int)vizPos.x - 1, (int)vizPos.y - 1, radius + 2, radius + 2, BLACK);
            DrawRectangle((int)vizPos.x, (int)vizPos.y, radius, radius, ORANGE);
        }
    }
#endif
}