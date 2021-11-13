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
    uint64_t           rtt_seed       {};
    dlb_rand32_t       rtt_rand       {};
    Tilemap           *map            {};
    // TODO: PlayerSystem
    Player            *player         {};
    Player             players[SERVER_MAX_PLAYERS]{};
    // TODO: SlimeSystem or EnemySystem
    std::vector<Slime> slimes         {};
    ParticleSystem     particleSystem {};
    MapSystem          mapSystem      {};

    World();
    ~World();
    const Vector3 GetWorldSpawn();
    Player *SpawnPlayer(const char *name);
    void GenerateEntities(const Slime *entities, size_t entityLength);
    void Sim(double now, double dt, const PlayerControllerState input, const SpriteDef *coinSpriteDef);
};