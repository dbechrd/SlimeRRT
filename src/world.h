#pragma once
#include "chat.h"
#include "controller.h"
#include "catalog/items.h"
#include "direction.h"
#include "item_system.h"
#include "item_world.h"
#include "particles.h"
#include "player.h"
#include "entities/entities.h"
#include "spycam.h"
#include "tilemap.h"
#include "world_snapshot.h"
#include "dlb_rand.h"
#include <vector>

struct NpcList {
    NPC    *data   {};
    size_t  length {};
};

struct World {
    uint64_t       rtt_seed       {};
    dlb_rand32_t   rtt_rand       {};
    uint32_t       tick           {};
    double         dtUpdate       {};
    // TODO: PlayerSystem
    uint32_t       playerId       {};
    Player         players        [SV_MAX_PLAYERS]{};
    PlayerInfo     playerInfos    [SV_MAX_PLAYERS]{};
    // TODO: NpcSystem
    struct {
        NPC slimes   [SV_MAX_NPC_SLIMES  ]{};
        NPC townfolk [SV_MAX_NPC_TOWNFOLK]{};
        NpcList byType[NPC::Type_Count]{
            {},
            { slimes,   ARRAY_SIZE(slimes   ) },
            { townfolk, ARRAY_SIZE(townfolk) }
        };
    } npcs;
    ItemSystem     itemSystem     {};
    LootSystem     lootSystem     {};
    MapSystem      mapSystem      {};
    Tilemap      & map            { mapSystem.Alloc() };
    ParticleSystem particleSystem {};
    ChatHistory    chatHistory    {};
    bool           peaceful       { true };
    bool           pvp            { true };

    World  (void);
    ~World (void);
    const Vector3 GetWorldSpawn(void);

    ////////////////////////////////////////////
    // vvv DO NOT HOLD A POINTER TO THESE! vvv
    //
    ErrorType   AddPlayerInfo        (const char *name, uint32_t nameLength, PlayerInfo **result);
    PlayerInfo *FindPlayerInfo       (uint32_t playerId);
    PlayerInfo *FindPlayerInfoByName (const char *name, size_t nameLength);
    void        RemovePlayerInfo     (uint32_t playerId);
    Player     *AddPlayer            (uint32_t playerId);
    Player     *FindPlayer           (uint32_t playerId);
    Player     *LocalPlayer          (void);
    Player     *FindPlayerByName     (const char *name, size_t nameLength);
    Player     *FindNearestPlayer    (Vector2 worldPos, float maxDist, Vector2 *dist = 0);
    void        RemovePlayer         (uint32_t playerId);
    ErrorType   SpawnSam             (NPC **result);
    ErrorType   SpawnNpc             (uint32_t id, NPC::Type type, Vector3 worldPos, NPC **result);
    NPC        *FindNpc              (uint32_t npcId);
    void        RemoveNpc            (uint32_t npcId);
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