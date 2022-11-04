#pragma once
#include "entities/npc.h"

class Slime {
public:
    uint32_t npcId        {};
    uint32_t id           {};
    double   randJumpIdle {};

    static void Init(NPC &npc);
    static void Update(NPC &npc, World &world, double dt);

private:
    static inline const char *LOG_SRC = "Slime";

    static bool Move(NPC &npc, double dt, Vector2 offset);
    static bool TryCombine(NPC &npc, NPC &other);
    static bool Attack(NPC &npc, double dt);
};
