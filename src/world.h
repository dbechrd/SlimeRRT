#pragma once
#include "chat.h"
#include "controller.h"
#include "item_catalog.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include "tilemap.h"
#include "world_snapshot.h"
#include "dlb_rand.h"
#include <vector>

struct World {
    uint64_t        rtt_seed       {};
    dlb_rand32_t    rtt_rand       {};
    uint32_t        tick           {};
    double          clockStart     {};
    double          clock          {};
    double          dtUpdate       {};
    Tilemap         *map           {};
    // TODO: PlayerSystem
    uint32_t        playerId       {};
    Player          players        [SERVER_MAX_PLAYERS]{};
    // TODO: SlimeSystem or EnemySystem
    Slime           slimes         [WORLD_ENTITIES_MAX]{};
    ParticleSystem  particleSystem {};
    MapSystem       mapSystem      {};
    ItemCatalog     itemCatalog    {};
    LootSystem      lootSystem     {};
    ChatHistory     chatHistory    {};

    World                       (void);
    ~World                      (void);
    const Vector3 GetWorldSpawn (void);

    ////////////////////////////////////////////
    // vvv DO NOT HOLD A POINTER TO THESE! vvv
    //
    Player *SpawnPlayer   (uint32_t playerId);
    Player *FindPlayer    (uint32_t playerId);
    void    DespawnPlayer (uint32_t playerId);

    Slime  *SpawnSlime    (uint32_t slimeId);
    Slime  *FindSlime     (uint32_t slimeId);
    void    DespawnSlime  (uint32_t slimeId);
    //
    // ^^^ DO NOT HOLD A POINTER TO THESE! ^^^
    ////////////////////////////////////////////

    void Simulate(double now, double dt);

    void GenerateSnapshot(WorldSnapshot &worldSnapshot);
    void Interpolate(double dtAccum, double interpLag);

private:
    void SimPlayers(double now, double dt);
    void SimSlimes(double now, double dt);
};