#pragma once
#include "loot_table.h"
#include "GLFW/glfw3.h"

struct Combat {
    float       hitPointsMax     {};
    float       hitPoints        {};
    float       meleeDamage      {};  // TODO: add min/max and randomly choose?
    double      attackStartedAt  {};
    double      attackDuration   {};
    double      diedAt           {};
    uint32_t    attackFrame      {};

    LootTableID lootTableId      {};
    bool        droppedHitLoot   {};  // loot on hit? could be fun for some slime items to fly off when attacking
    bool        droppedDeathLoot {};  // loot on death, proper loot roll

    inline float DealDamage(float damage)
    {
        const float dealt = CLAMP(damage, 0.0f, hitPoints);
        hitPoints -= dealt;
        if (!hitPoints) {
            diedAt = glfwGetTime();
        }
        return dealt;
    }
};