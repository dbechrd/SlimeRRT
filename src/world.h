#pragma once
#include "controller.h"
#include "player.h"
#include "tilemap.h"
#include "slime.h"
#include "dlb_rand.h"
#include <vector>

struct World {
    uint64_t           rtt_seed {};
    dlb_rand32_t       rtt_rand {};
    Tilemap            map      {};
    Tileset           *tileset  {};
    Player            *player   {};
    std::vector<Slime> slimes   {};

    World();
    ~World();
    const Vector3 World::GetWorldSpawn();
    void Sim(double now, double dt, const PlayerControllerState input, const SpriteDef *coinSpriteDef);
};