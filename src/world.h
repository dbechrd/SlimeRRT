#pragma once
#include "controller.h"
#include "item_catalog.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include "tilemap.h"
#include "dlb_rand.h"
#include <vector>

struct NetMessage_Input;

struct World {
    uint64_t        rtt_seed       {};
    dlb_rand32_t    rtt_rand       {};
    uint32_t        tick           {};
    Tilemap         *map           {};
    // TODO: PlayerSystem
    uint32_t        playerId       {};
    Player          players        [SERVER_MAX_PLAYERS]{};
    // TODO: SlimeSystem or EnemySystem
    uint32_t        slimeCount     {};
    Slime           slimes         [WORLD_ENTITIES_MAX]{};
    ParticleSystem  particleSystem {};
    MapSystem       mapSystem      {};
    ItemCatalog     itemCatalog    {};
    LootSystem      lootSystem     {};

    World                       (void);
    ~World                      (void);
    const Vector3 GetWorldSpawn (void);

    ////////////////////////////////////////////
    // vvv DO NOT HOLD A POINTER TO THESE! vvv
    //
    Player *SpawnPlayer   (void);
    Player *FindPlayer    (uint32_t playerId);
    void    DespawnPlayer (uint32_t playerId);

    Slime  *SpawnSlime    (void);
    Slime  *FindSlime     (uint32_t slimeId);
    void    DespawnSlime  (uint32_t slimeId);
    //
    // ^^^ DO NOT HOLD A POINTER TO THESE! ^^^
    ////////////////////////////////////////////

    void    SimPlayer     (double now, double dt, Player &player, const NetMessage_Input &input);
    void    SimSlimes     (double now, double dt);
};

struct WorldSnapshot {
    uint32_t tick    {};
    Player   players [SERVER_MAX_PLAYERS]{};
    Slime    slimes  [WORLD_ENTITIES_MAX]{};
};