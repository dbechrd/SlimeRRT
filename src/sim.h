#pragma once
#include "controller.h"

void sim(double now, double dt, PlayerControllerState input, struct Player *player, struct Tilemap *map,
    struct Slime *slimes, size_t slimeCount, const SpriteDef *coinSprite);