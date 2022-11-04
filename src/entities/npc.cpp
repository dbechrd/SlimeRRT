#include "npc.h"
#include "draw_command.h"
#include "healthbar.h"
#include "helpers.h"
#include "maths.h"
#include "player.h"
#include "shadow.h"
#include "spritesheet.h"
#include "dlb_rand.h"
#include "particles.h"

void Entity::SetName(const char *newName, uint32_t newNameLen)
{
    memset(name, 0, nameLength);
    nameLength = MIN(newNameLen, sizeof(name));
    memcpy(name, newName, nameLength);
}

Vector3 Entity::WorldCenter(World &world, EntityID entityId)
{
    DLB_ASSERT(entityId);
    Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
    Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
    DLB_ASSERT(body3d);
    DLB_ASSERT(sprite);

    const Vector3 worldC = v3_add(body3d->WorldPosition(), sprite_center(*sprite));
    return worldC;
}

Vector3 Entity::WorldTopCenter3D(World &world, EntityID entityId)
{
    DLB_ASSERT(entityId);
    Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
    Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
    DLB_ASSERT(body3d);
    DLB_ASSERT(sprite);

    const Vector3 worldTopC = v3_add(body3d->WorldPosition(), sprite_top_center(*sprite));
    return worldTopC;
}

Vector2 Entity::WorldTopCenter2D(World &world, EntityID entityId)
{
    DLB_ASSERT(entityId);

    Vector3 worldTopC3D = WorldTopCenter3D(world, entityId);
    Vector2 worldTopC2D = { worldTopC3D.x, worldTopC3D.y - worldTopC3D.z };
    //worldTopC2D.x = floorf(worldTopC2D.x);
    //worldTopC2D.y = floorf(worldTopC2D.y);
    return worldTopC2D;
}

Vector3 Entity::GetAttachPoint(World &world, EntityID entityId, AttachType attachType)
{
    DLB_ASSERT(entityId);
    DLB_ASSERT(attachType >= 0);
    DLB_ASSERT(attachType < Attach_Count);
    Attach *attach = (Attach *)world.facetDepot.FacetFind(entityId, Facet_Attach);
    DLB_ASSERT(attach);

    Vector3 attachPoint = {};
    if (attach) {
        attachPoint = v3_add(WorldCenter(world, entityId), attach->points[attachType]);
    }
    return attachPoint;
}

void Entity::Update(World &world, EntityID entityId, double dt)
{
    DLB_ASSERT(entityId);
    Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
    Combat *combat = (Combat *)world.facetDepot.FacetFind(entityId, Facet_Combat);
    Entity *entity = (Entity *)world.facetDepot.FacetFind(entityId, Facet_Entity);
    Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
    DLB_ASSERT(body3d);
    DLB_ASSERT(combat);
    DLB_ASSERT(entity);
    DLB_ASSERT(sprite);

    DLB_ASSERT(!entity->despawnedAt);

    // Update animations
    combat->Update(dt);
    sprite_update(*sprite, dt);

    if (!combat->diedAt) {
        switch (entity->entityType) {
            case Entity_Slime: Slime::Update(world, entityId, dt); break;
        }
        body3d->Update(dt);
    } else if (!body3d->Resting()) {
        body3d->Update(dt);
    }

    if (g_clock.server && combat->diedAt && !combat->droppedDeathLoot) {
        world.lootSystem.SV_RollDrops(combat->lootTableId, [&](ItemStack dropStack) {
            world.itemSystem.SpawnItem(WorldCenter(world, entityId), dropStack.uid, dropStack.count);
        });

        combat->droppedDeathLoot = true;
    }
}

// TODO(cleanup): Just call body3d->depth directly wherever this is being used
float Entity::Depth(World &world, EntityID entityId)
{
    DLB_ASSERT(entityId);
    Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
    DLB_ASSERT(body3d);

    float depth = body3d->GroundPosition().y;
    return depth;
}

bool Entity::Cull(World &world, EntityID entityId, const Rectangle &cullRect)
{
    DLB_ASSERT(entityId);
    Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
    Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
    DLB_ASSERT(body3d);
    DLB_ASSERT(sprite);

    bool cull = sprite_cull_body(*sprite, *body3d, cullRect);
    return cull;
}

void Entity::DrawSwimOverlay(World &world, EntityID entityId)
{
    DLB_ASSERT(entityId);
    Entity *entity = (Entity *)world.facetDepot.FacetFind(entityId, Facet_Entity);
    DLB_ASSERT(entity);
    if (!entity->entityType == Entity_Player) {
        return;
    }

    Attach *attach = (Attach *)world.facetDepot.FacetFind(entityId, Facet_Attach);
    Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
    DLB_ASSERT(attach);
    DLB_ASSERT(body3d);

    Vector2 groundPos = body3d->GroundPosition();
    DLB_ASSERT(isfinite(groundPos.x));
    DLB_ASSERT(isfinite(groundPos.y));
    const Tile *tileLeft = world.map.TileAtWorld(groundPos.x - 15.0f, groundPos.y);
    const Tile *tileRight = world.map.TileAtWorld(groundPos.x + 15.0f, groundPos.y);
    if ((tileLeft && tileLeft->type == TileType_Water) ||
        (tileRight && tileRight->type == TileType_Water)) {
        auto tilesetId = world.map.tilesetId;
        Tileset &tileset = g_tilesets[(size_t)tilesetId];
        Rectangle tileRect = tileset_tile_rect(tilesetId, TileType_Water);
        DLB_ASSERT(tileRect.width == TILE_W);
        DLB_ASSERT(tileRect.height == TILE_W);

        Vector3 gut3d = attach->points[Attach_Gut];
        Vector2 gut2d = { gut3d.x, gut3d.y - gut3d.z + 10.0f };
        float offsetX = fmodf(gut2d.x, TILE_W);
        float offsetY = fmodf(gut2d.y, TILE_W);
        if (offsetX < 0) offsetX += TILE_W;
        if (offsetY < 0) offsetY += TILE_W;

        Rectangle dstTopLeft{
            gut2d.x - offsetX - ((offsetX < (TILE_W / 2)) ? TILE_W : 0),
            gut2d.y,
            TILE_W,
            TILE_W - offsetY
        };

        Rectangle dstTopRight = dstTopLeft;
        dstTopRight.x += TILE_W;

        Rectangle srcTop = tileRect;
        srcTop.y += offsetY;
        srcTop.height -= offsetY;

        Rectangle dstBotLeft{
            gut2d.x - offsetX - ((offsetX < (TILE_W / 2)) ? TILE_W : 0),
            gut2d.y - offsetY + TILE_W,
            TILE_W,
            TILE_W
        };

        Rectangle dstBotRight = dstBotLeft;
        dstBotRight.x += TILE_W;

        float minXWater = FLT_MAX;
        float maxXWater = FLT_MIN;

#define CHECK_AND_DRAW(src, dst) \
            tile = world.map.TileAtWorld((dst).x, (dst).y);                                  \
            if (tile && tile->type == TileType_Water) {                                   \
                DrawTexturePro(tileset.texture, (src), (dst), { 0, 0 }, 0, Fade(WHITE, 0.8f)); \
                minXWater = MIN(minXWater, (dst).x);                                           \
                maxXWater = MAX(maxXWater, (dst).x + (dst).width);                             \
            }

        const Tile *tile = 0;
        CHECK_AND_DRAW(srcTop, dstTopLeft);
        CHECK_AND_DRAW(srcTop, dstTopRight);
        CHECK_AND_DRAW(tileRect, dstBotLeft);
        CHECK_AND_DRAW(tileRect, dstBotRight);

#undef CHECK_AND_DRAW

        const Tile *playerGutTile = world.map.TileAtWorld(gut2d.x, gut2d.y);
        if (playerGutTile && playerGutTile->type == TileType_Water) {
#if 0
            // TODO: Bubbles when swimming??
            Rectangle bubblesDstTopMid{
                gut2d.x - 20.0f,
                gut2d.y,
                40.0f,
                8.0f
            };

            Rectangle bubblesSrcTop = tileRect;
            bubblesSrcTop.y += offsetY;
            bubblesSrcTop.height -= offsetY;
#endif

            //DrawCircleV({ minXWater, playerGut2D.y }, 2.0f, PINK);
            //DrawCircleV({ maxXWater, playerGut2D.y }, 2.0f, PINK);

            const float radiusDelta = (entity->moveState != Entity::Move_Idle)
                ? (sinf((float)g_clock.now * 8) * 3)
                : 0.0f;
            const float radius = 20.0f + radiusDelta;
            DrawEllipse((int)gut2d.x, (int)gut2d.y, radius, radius * 0.5f, Fade(SKYBLUE, 0.4f));
        }
    }
}

void Entity::Draw(World &world, EntityID entityId, Vector2 at)
{
    DLB_ASSERT(entityId);
    UNUSED(at);

    Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
    Combat *combat = (Combat *)world.facetDepot.FacetFind(entityId, Facet_Combat);
    Entity *entity = (Entity *)world.facetDepot.FacetFind(entityId, Facet_Entity);
    Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);

    DLB_ASSERT(body3d);
    DLB_ASSERT(combat);
    DLB_ASSERT(entity);
    DLB_ASSERT(sprite);

    DLB_ASSERT(!entity->despawnedAt);

    // HACK: Don't draw players who have left the game
    //PlayerInfo *playerInfo = world.FindPlayerInfo(id);
    //if (!playerInfo) return;

    const Vector3 groundPos = body3d->GroundPosition3();

    // Usually 1.0, fades to zero after death
    const float sinceDeath = combat->diedAt ? (float)((g_clock.now - combat->diedAt) / CL_NPC_CORPSE_LIFETIME) : 0;
    const float deadAlpha = CLAMP(1.0f - sinceDeath, 0.5f, 1.0f);

    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;

    // TODO: This probably needs to exist on the sprite as property if we keep it around
    const Vector2 shadowOffset = { 0, -8 };

    Shadow::Draw(groundPos, 16.0f * sprite->scale, shadowOffset.y * sprite->scale, deadAlpha);
    sprite_draw_body(*sprite, *body3d, Fade(WHITE, 0.7f * deadAlpha));

    if (entity->entityType == Entity_Player) {
        DrawSwimOverlay(world, entityId);

#if CL_DEMO_SNAPSHOT_RADII
        {
            const Vector2 visPos = body3d->VisualPosition();
            DrawCircleLines((int)visPos.x, (int)visPos.y, SV_NPC_NEARBY_THRESHOLD, GREEN);
            DrawCircleLines((int)visPos.x, (int)visPos.y, SV_ENEMY_MIN_SPAWN_DIST, YELLOW);
            DrawCircleLines((int)visPos.x, (int)visPos.y, SV_ENEMY_DESPAWN_RADIUS, RED);
        }
#endif
    }

#if CL_DEMO_SNAPSHOT_RADII
    // DEBUG: Draw stale visual marker if no snapshot received in a while
    if (body3d->positionHistory.Count()) {
        auto &lastPos = body3d->positionHistory.Last();
        if (g_clock.now - lastPos.serverTime > CL_NPC_STALE_LIFETIME) {
            Vector2 vizPos = body3d->VisualPosition();
            const int radius = 7;
            DrawRectangle((int)vizPos.x - 1, (int)vizPos.y - 1, radius + 2, radius + 2, BLACK);
            DrawRectangle((int)vizPos.x, (int)vizPos.y, radius, radius, RED);
        }
    }
#endif

    if (!combat->diedAt) {
        Vector2 topCenter = Entity::WorldTopCenter2D(world, entityId);
        HealthBar::Draw(topCenter, entity->name, *combat, entityId);
    }
}