#pragma once
#include "player.h"
#include "tilemap.h"
#include "slime.h"
//#include <array>

typedef struct World {
    Player *player;
    Tilemap *map;
    Slime *slimes;
    //std::array<Slime, 100> slimess;
} World;