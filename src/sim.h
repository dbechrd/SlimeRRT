#pragma once
#include "controller.h"
#include "world.h"

void sim(double now, double dt, const PlayerControllerState input, World &world, const SpriteDef *coinSpriteDef);