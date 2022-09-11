#include "enemy.h"
#include "draw_command.h"
#include "healthbar.h"
#include "helpers.h"
#include "maths.h"
#include "monster/monster.h"
#include "shadow.h"
#include "spritesheet.h"
#include "dlb_rand.h"
#include <cassert>
#include "particles.h"

void Enemy::Init(Type type)
{
    assert(!sprite.scale);

    this->type = type;
    SetName(CSTR("Enemy"));

    body.speed = 1.0f;
    body.drag = 0.95f;
    body.friction = 0.95f;

    combat.level = 1;
    combat.hitPointsMax = 1.0f;
    combat.hitPoints = combat.hitPointsMax;
    combat.meleeDamage = 1.0f;
    combat.xpMin = 1;
    combat.xpMax = 1;
    combat.lootTableId = LootTableID::LT_Empty;

    sprite.scale = 1.0f;
}

void Enemy::SetName(const char *newName, uint32_t newNameLen)
{
    memset(name, 0, nameLength);
    nameLength = MIN(newNameLen, USERNAME_LENGTH_MAX);
    memcpy(name, newName, nameLength);
    if (nameLength < USERNAME_LENGTH_MAX) {
        snprintf(name + nameLength, USERNAME_LENGTH_MAX - nameLength, " %d", id);
    }
}

Vector3 Enemy::WorldCenter(void) const
{
    const Vector3 worldC = v3_add(body.WorldPosition(), sprite_center(sprite));
    return worldC;
}

Vector3 Enemy::WorldTopCenter(void) const
{
    const Vector3 worldTopC = v3_add(body.WorldPosition(), sprite_top_center(sprite));
    return worldTopC;
}

void Enemy::UpdateDirection(Vector2 offset)
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

void Enemy::Update(World &world, double dt)
{
    if (combat.diedAt) {
        if (combat.droppedDeathLoot) {
            return;
        }

        uint32_t coins = world.lootSystem.RollCoins(combat.lootTableId, (int)sprite.scale);
        assert(coins);

        //ItemType coinType{};
        //const float chance = dlb_rand32f_range(0, 1);
        //if (chance < 100.0f / 111.0f) {
        //    coinType = ItemType::Currency_Copper;
        //} else if (chance < 10.0f / 111.0f) {
        //    coinType = ItemType::Currency_Silver;
        //} else {
        //    coinType = ItemType::Currency_Gilded;
        //}

        world.lootSystem.RollDrops(combat.lootTableId, [&](ItemStack dropStack) {
            world.itemSystem.SpawnItem(WorldCenter(), dropStack.uid, dropStack.count);
        });
        combat.droppedDeathLoot = true;
    } else {
        switch (type) {
            case Type_Slime: Monster::slime_update(*this, dt, world); break;
            default: break;
        }
    }

    body.Update(dt);
    sprite_update(sprite, dt);
    combat.Update(dt);
}

float Enemy::Depth(void) const
{
    return body.GroundPosition().y;
}

bool Enemy::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void Enemy::Draw(World &world)
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