#pragma once
#include "entities/npc.h"

namespace Slime {
    const char *LOG_SRC = "Slime";

    ErrorType Init       (World &world, EntityID entityId);
    bool      TryCombine (World &world, EntityID entityId, EntityID otherId);
    bool      Move       (World &world, EntityID entityId, double dt, Vector2 offset);
    bool      Attack     (World &world, EntityID entityId, double dt);
    void      Update     (World &world, EntityID entityId, double dt);
}
