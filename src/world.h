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

struct World {
    uint64_t       rtt_seed       {};
    dlb_rand32_t   rtt_rand       {};
    uint32_t       tick           {};
    double         clockStart     {};
    double         clock          {};
    double         dtUpdate       {};
    Tilemap        *map           {};
    // TODO: PlayerSystem
    uint32_t       nextPlayerId   {};
    uint32_t       playerId       {};
    Player         players        [SV_MAX_PLAYERS]{};
    // TODO: SlimeSystem or EnemySystem
    Slime          slimes         [SV_MAX_SLIMES]{};
    ItemWorld      items          [SV_MAX_ITEMS]{};

    ChatHistory    chatHistory    {};
    ItemSystem     itemSystem     {};
    LootSystem     lootSystem     {};
    MapSystem      mapSystem      {};
    ParticleSystem particleSystem {};

    World  (void);
    ~World (void);
    const Vector3 GetWorldSpawn(void);

    ////////////////////////////////////////////
    // vvv DO NOT HOLD A POINTER TO THESE! vvv
    //
    Player *AddPlayer     (uint32_t playerId);
    Player *FindPlayer    (uint32_t playerId);
    void    DespawnPlayer (uint32_t playerId);
    void    RemovePlayer  (uint32_t playerId);

    Slime  &SpawnSam      (void);
    Slime  *SpawnSlime    (uint32_t slimeId);
    Slime  *FindSlime     (uint32_t slimeId);
    void    DespawnSlime  (uint32_t slimeId);

    Player *FindClosestPlayer(Vector2 worldPos, float maxDist);
    //
    // ^^^ DO NOT HOLD A POINTER TO THESE! ^^^
    ////////////////////////////////////////////

    void Simulate(double dt);
    void Interpolate(double renderAt);

    void EnableCulling(Rectangle cullRect);
    size_t DrawMap(int zoomMipLevel);
    void DrawItems(void);
    void DrawEntities(void);
    void DrawParticles(void);
    void DrawFlush(void);

private:
    void SimPlayers(double dt);
    void SimSlimes(double dt);
    void SimItems(double dt);
    bool InterpolateBody(Body3D &body, double renderAt, Direction direction);

    DrawList drawList{};
    bool CullTile(Vector2 tilePos, int zoomMipLevel);
};