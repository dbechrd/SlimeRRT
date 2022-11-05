#pragma once
#include "../helpers.h"

struct World;

namespace Slime {
    const char *LOG_SRC = "Slime";

    ErrorType Init       (World &world, EntityID entityId);
    void      Update     (World &world, EntityID entityId, double dt);
}
