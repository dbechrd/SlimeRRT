#pragma once
#include "../helpers.h"

struct World;

namespace Townfolk
{
    const char *LOG_SRC = "Townfolk";

    ErrorType Init   (World &world, EntityID entityId);
    void      Update (World &world, EntityID entityId, double dt);
}