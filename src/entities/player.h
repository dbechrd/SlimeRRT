#pragma once
#include "../helpers.h"

struct World;
// HACK: Player::Update SHOULD NOT be doing collision detection.. WUT!?
struct Tilemap;

namespace Player
{
    const char *LOG_SRC = "Player";

    ErrorType Init   (World &world, EntityID entityId);
    void      Update (World &world, EntityID entityId, InputSample &input, Tilemap &map);
};
