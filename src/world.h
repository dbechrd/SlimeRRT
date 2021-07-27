#pragma once
#include "player.h"
#include "tilemap.h"
#include "slime.h"
#include "dlb_rand.h"
//#include <array>

struct Args {
    bool server;
};

struct World {
    Args args;
    Player *player;
    Tilemap *map;
    Slime *slimes;
    //std::array<Slime, 100> slimess;
    uint64_t rtt_seed;
    dlb_rand32_t rtt_rand;
};
extern World g_world;