#pragma once
#include "controller.h"
#include "net_message.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include "tilemap.h"
#include "dlb_rand.h"
#include <vector>

struct World {
    uint64_t        rtt_seed       {};
    dlb_rand32_t    rtt_rand       {};
    Tilemap         *map           {};
    // TODO: PlayerSystem
    size_t          playerCount    {};
    Player          players[SERVER_MAX_PLAYERS]{};
    // TODO: SlimeSystem or EnemySystem
    size_t          slimeCount     {};
    Slime           slimes[WORLD_ENTITIES_MAX]{};
    ParticleSystem  particleSystem {};
    MapSystem       mapSystem      {};

    World                          (void);
    ~World                         (void);
    const Vector3 GetWorldSpawn    (void);
    void          InitPlayer       (Player &player, const char *name, size_t nameLength);
    Player       *SpawnPlayer      (const char *name, size_t nameLength);
    void          SyncPlayers      (const Player *players, size_t playersLength);
    void          SyncEntities     (const Slime *entities, size_t entityLength);
    void          Sim              (double now, double dt, const PlayerControllerState input);
    void          SimSlimes        (double now, double dt);
};