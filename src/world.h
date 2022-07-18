#pragma once
#include "chat.h"
#include "controller.h"
#include "catalog/items.h"
#include "direction.h"
#include "item_system.h"
#include "item_world.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include "tilemap.h"
#include "world_snapshot.h"
#include "dlb_rand.h"
#include <vector>

struct Spycam;

struct World {
    uint64_t       rtt_seed       {};
    dlb_rand32_t   rtt_rand       {};
    uint32_t       tick           {};
    double         dtUpdate       {};
    Tilemap        *map           {};
    // TODO: PlayerSystem
    uint32_t       nextPlayerId   {};
    uint32_t       playerId       {};
    Player         players        [SV_MAX_PLAYERS]{};
    // TODO: SlimeSystem or EnemySystem
    Slime          slimes         [SV_MAX_SLIMES]{};
    ItemSystem     itemSystem     {};
    LootSystem     lootSystem     {};
    MapSystem      mapSystem      {};
    ParticleSystem particleSystem {};
    ChatHistory    chatHistory    {};
    bool           peaceful       {};
    bool           pvp            {};

    World  (void);
    ~World (void);
    const Vector3 GetWorldSpawn(void);

    ////////////////////////////////////////////
    // vvv DO NOT HOLD A POINTER TO THESE! vvv
    //
    Player *AddPlayer         (uint32_t playerId);
    Player *FindPlayer        (uint32_t playerId);
    Player *FindPlayerByName  (const char *name, size_t nameLength);
    Player *FindClosestPlayer (Vector2 worldPos, float maxDist, float *distSq);
    void    RemovePlayer      (uint32_t playerId);

    Slime  &SpawnSam    (void);
    Slime  *SpawnSlime  (uint32_t slimeId, Vector2 origin);
    Slime  *FindSlime   (uint32_t slimeId);
    void    RemoveSlime (uint32_t slimeId);
    //
    // ^^^ DO NOT HOLD A POINTER TO THESE! ^^^
    ////////////////////////////////////////////

    void SV_Simulate(double dt);
    void DespawnDeadEntities(void);

    void CL_Interpolate(double renderAt);
    void CL_Extrapolate(double dt);
    void CL_Animate(double dt);
    void CL_DespawnStaleEntities(void);

    void EnableCulling(Rectangle cullRect);
    size_t DrawMap(const Spycam &spycam);
    void DrawItems(void);
    void DrawEntities(void);
    void DrawParticles(void);
    void DrawFlush(void);

private:
    void SV_SimPlayers(double dt);
    void SV_SimSlimes(double dt);
    void SV_SimItems(double dt);

    bool CL_InterpolateBody(Body3D &body, double renderAt, Direction &direction);

    DrawList drawList{};
    bool CullTile(Vector2 tilePos, int zoomMipLevel);
};