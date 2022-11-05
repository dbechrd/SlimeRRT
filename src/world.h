#pragma once
#include "chat.h"
#include "controller.h"
#include "catalog/items.h"
#include "direction.h"
#include "entities/entities.h"
#include "entities/facet.h"
#include "item_system.h"
#include "particles.h"
#include "player.h"
#include "spycam.h"
#include "tilemap.h"
#include "world_item.h"
#include "world_snapshot.h"
#include "dlb_rand.h"
#include <algorithm>
#include <vector>

// TODO: Put this somewhere else
// TODO: Silly idea: Make everything a type/count pair that goes
// into some invisible inventory slot. Then just serialize the
// relevant "items" in an NPC's "inventory" over the network to
// replicate the whole thing. Inventory = components!?
struct Stats {
    uint32_t xp              {};
    float    damageDealt     {};
    float    kmWalked        {};
    uint32_t entitiesSlain   [Entity_Count]{};
    uint32_t timesFistSwung  {};
    uint32_t timesSwordSwung {};
    //uint32_t coinsCollected  {};  // TODO: Money earned via selling? Or something else cool
};

// TODO: Put these somewhere else..
enum AttachType {
    Attach_Gut,
    Attach_Count
};
struct Attach : public Facet {
    Vector3 points[Attach_Count]{};
};

// TODO: Refactor WorldItems out of ItemSystem into EntityCollector
// TODO: ResourceCatalog byType/byId for shared resources (textures, sprites, etc.)
struct FacetDepot {
    // NOTE: Slot 0 is reserved for the EMPTY facet in each pool
    std::vector<Attach>    attachPool    {{}};
    std::vector<Body3D>    body3dPool    {{}};
    std::vector<Combat>    combatPool    {{}};
    std::vector<Entity>    entityPool    {{}};
    std::vector<Inventory> inventoryPool {{}};
    std::vector<Sprite>    spritePool    {{}};
    std::unordered_map<EntityID, size_t> poolIndexByEntityID[Facet_Count]{};
    std::vector<EntityID>entityIdsByType[Entity_Count]{};

    ErrorType FacetAlloc(EntityID entityId, FacetType type, Facet **result)
    {
        DLB_ASSERT(type >= 0);
        DLB_ASSERT(type < Facet_Count);

        size_t poolIdx = poolIndexByEntityID[type][entityId];
        if (poolIdx) {
            E_ERROR_RETURN(ErrorType::AllocFailed_Duplicate, "Entity id %u already has facet of type %u!", entityId, type);
        }

        Facet *facet = 0;
        switch (type) {
            case Facet_Attach: {
                Facet *facet = &attachPool.emplace_back();
                poolIdx = attachPool.size() - 1;
                break;
            }
            case Facet_Body3D: {
                Facet *facet = &body3dPool.emplace_back();
                poolIdx = body3dPool.size() - 1;
                break;
            }
            case Facet_Combat: {
                Facet *facet = &combatPool.emplace_back();
                poolIdx = combatPool.size() - 1;
                break;
            }
            case Facet_Entity: {
                Facet *facet = &entityPool.emplace_back();
                poolIdx = entityPool.size() - 1;
                break;
            }
            case Facet_Inventory: {
                Facet *facet = &inventoryPool.emplace_back();
                poolIdx = inventoryPool.size() - 1;
                break;
            }
            case Facet_Sprite: {
                Facet *facet = &spritePool.emplace_back();
                poolIdx = spritePool.size() - 1;
                break;
            }
            default: {
                E_ERROR_RETURN(ErrorType::UnknownType, "Unrecognized facet type %u", type);
                break;
            }
        }
        DLB_ASSERT(facet);
        DLB_ASSERT(poolIdx);

        facet->entityId = entityId;
        facet->facetType = type;
        poolIndexByEntityID[type][entityId] = poolIdx;
        if (result) *result = facet;
        return ErrorType::Success;
    }

    Facet *FacetFind(EntityID entityId, FacetType type)
    {
        DLB_ASSERT(type >= 0);
        DLB_ASSERT(type < Facet_Count);

        size_t poolIdx = poolIndexByEntityID[type][entityId];
        if (poolIdx) {
            switch (type) {
                case Facet_Attach:    return &attachPool[poolIdx];
                case Facet_Body3D:    return &body3dPool[poolIdx];
                case Facet_Combat:    return &combatPool[poolIdx];
                case Facet_Entity:    return &entityPool[poolIdx];
                case Facet_Inventory: return &inventoryPool[poolIdx];
                case Facet_Sprite:    return &spritePool[poolIdx];
            }
        }
        return 0;
    }

    void FacetFree(EntityID entityId, FacetType type)
    {
        DLB_ASSERT(type >= 0);
        DLB_ASSERT(type < Facet_Count);

        size_t poolIdx = poolIndexByEntityID[type][entityId];
        if (!poolIdx) {
            return;
        }

        switch (type) {
            case Facet_Attach: {
                attachPool[poolIdx] = std::move(attachPool.back());
                attachPool.pop_back();
                break;
            }
            case Facet_Body3D: {
                body3dPool[poolIdx] = std::move(body3dPool.back());
                body3dPool.pop_back();
                break;
            }
            case Facet_Combat: {
                combatPool[poolIdx] = std::move(combatPool.back());
                combatPool.pop_back();
                break;
            }
            case Facet_Entity: {
                entityPool[poolIdx] = std::move(entityPool.back());
                entityPool.pop_back();
                break;
            }
            case Facet_Inventory: {
                inventoryPool[poolIdx] = std::move(inventoryPool.back());
                inventoryPool.pop_back();
                break;
            }
            case Facet_Sprite: {
                spritePool[poolIdx] = std::move(spritePool.back());
                spritePool.pop_back();
                break;
            }
        }
    }

    ErrorType EntityAlloc(EntityID entityId, EntityType type, Entity **result)
    {
        Entity *entity = 0;
        E_ERROR_RETURN(FacetAlloc(entityId, Facet_Entity, (Facet **)entity),
            "Failed to allocate entity id %u type %u", entityId, type);
        entityIdsByType[type].emplace_back(entityId);
        entity->entityType = type;
        if (result) *result = entity;
    }

    Entity *EntityFind(EntityID entityId)
    {
        Entity *entity = 0;
        size_t poolIdx = poolIndexByEntityID[Facet_Entity][entityId];
        if (poolIdx) {
            entity = &entityPool[poolIdx];
            DLB_ASSERT(entity->entityId == entityId);
        }
        return entity;
    }

    void EntityFree(EntityID entityId) {
        E_DEBUG("EntityFree [%u]", entityId);

        Entity *entity = EntityFind(entityId);
        if (entity) {
            auto &vec = entityIdsByType[entity->entityType];
            vec.erase(std::remove(vec.begin(), vec.end(), entityId), vec.end());
        } else {
            E_WARN("No entity facet found for id %u. Removing other facets anyway.", entityId);
        }

        for (int type = 0; type < Facet_Count; type++) {
            FacetFree(entityId, (FacetType)type);
        }
    }

private:
    const char *LOG_SRC = "EntityOwner";
};

struct World {
    uint64_t       rtt_seed       {};
    dlb_rand32_t   rtt_rand       {};
    uint32_t       tick           {};
    double         dtUpdate       {};
    EntityID       playerId       {};
    PlayerInfo     playerInfos    [SV_MAX_PLAYERS]{};
    FacetDepot     facetDepot     {};
    ItemSystem     itemSystem     {};
    LootSystem     lootSystem     {};
    MapSystem      mapSystem      {};
    Tilemap      & map            { mapSystem.Alloc() };
    ParticleSystem particleSystem {};
    ChatHistory    chatHistory    {};
    bool           peaceful       { false };
    bool           pvp            { true };

    World  (void);
    ~World (void);
    const Vector3 GetWorldSpawn(void);

    ////////////////////////////////////////////
    // vvv DO NOT HOLD A POINTER TO THESE! vvv
    //
    ErrorType   AddPlayerInfo        (const char *name, uint32_t nameLength, PlayerInfo **result);
    PlayerInfo *FindPlayerInfo       (EntityID entityId);
    PlayerInfo *FindPlayerInfoByName (const char *name, size_t nameLength);
    void        RemovePlayerInfo     (EntityID entityId);
    EntityID    PlayerFindOrCreate   (EntityID entityId);
    EntityID    PlayerFindByName     (const char *name, size_t nameLength);
    EntityID    PlayerFindNearest    (Vector2 worldPos, float maxDist, Vector2 *toPlayer = 0);
    void        RemovePlayer         (EntityID entityId);
    ErrorType   SpawnSam             (void);
    ErrorType   SpawnEntity          (EntityID entityId, EntityType type, Vector3 worldPos, EntityID *result);
    //
    // ^^^ DO NOT HOLD A POINTER TO THESE! ^^^
    ////////////////////////////////////////////

    void   SV_Simulate              (double dt);
    void   SV_DespawnDeadEntities   (void);

    void   CL_Interpolate          (double renderAt);
    void   CL_Extrapolate          (double dt);
    void   CL_Animate              (double dt);
    void   CL_DespawnStaleEntities (void);

    void   EnableCulling (Rectangle cullRect);
    size_t DrawMap       (const Spycam &spycam);
    void   DrawItems     (void);
    void   DrawEntities  (void);
    void   DrawParticles (void);
    void   DrawFlush     (void);

private:
    const char *LOG_SRC = "World";
    void SV_SimPlayers (double dt);
    void SV_SimNpcs    (double dt);
    void SV_SimItems   (double dt);

    bool CL_InterpolateBody(Body3D &body, double renderAt, Direction &direction);

    DrawList drawList{};
    bool CullTile(Vector2 tilePos, int zoomMipLevel);
};