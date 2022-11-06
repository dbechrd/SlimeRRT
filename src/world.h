#pragma once
#include "chat.h"
#include "controller.h"
#include "catalog/items.h"
#include "direction.h"
#include "entities/attach.h"
#include "entities/facet.h"
#include "entities/facet_depot.h"
#include "helpers.h"
#include "item_system.h"
#include "particles.h"
#include "player_info.h"
#include "spycam.h"
#include "tilemap.h"
#include "world_item.h"
#include "world_snapshot.h"
#include "dlb_rand.h"
#include <algorithm>
#include <vector>

struct World {
    uint64_t       rtt_seed       {};
    dlb_rand32_t   rtt_rand       {};
    uint32_t       tick           {};
    double         dtUpdate       {};
    EntityID       playerId       {};
    PlayerInfo     playerInfos    [SV_MAX_PLAYERS]{};  // TODO: Make this a map+vector
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
    ErrorType   AddPlayerInfo        (PlayerInfo **result);
    PlayerInfo *FindPlayerInfo       (EntityID entityId);
    PlayerInfo *FindPlayerInfoByName (const char *name, size_t nameLength);
    void        RemovePlayerInfo     (EntityID entityId);
    EntityID    PlayerFindByName     (const char *name, size_t nameLength);
    EntityID    PlayerFindNearest    (Vector2 worldPos, float maxDist, Vector2 *toPlayer = 0, bool includeDead = false);
    ErrorType   SpawnSam             (void);
    ErrorType   SpawnEntity          (EntityID entityId, EntityType type, Vector3 worldPos, Entity **result);
    ErrorType   FindOrSpawnEntity    (EntityID entityId, EntityType type, Entity **result);
    //
    // ^^^ DO NOT HOLD A POINTER TO THESE! ^^^
    ////////////////////////////////////////////

    void   SV_Simulate              (double dt);
    void   SV_DespawnDeadEntities   (void);

    void   CL_Interpolate           (double renderAt);
    void   CL_Extrapolate           (double dt);
    void   CL_Animate               (double dt);
    void   CL_FreeDespawnedEntities (void);

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