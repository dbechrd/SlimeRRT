#pragma once
#include "controller.h"

struct Player;
struct Slime;
struct Tilemap;

void sim(double now, double dt, PlayerControllerState input, struct Player *player, struct Tilemap *map,
    struct Slime *slimes, const SpriteDef *coinSprite);