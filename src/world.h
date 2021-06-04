#pragma once
#include "player.h"
#include "tilemap.h"
#include "slime.h"
//#include <array>

struct World {
    Player *player;
    Tilemap *map;
    Slime *slimes;
    //std::array<Slime, 100> slimess;
    uint64_t rtt_seed;
    dlb_rand32_t rtt_rand;
};