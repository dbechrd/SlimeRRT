#pragma once
#include "loot_table.h"
#include "GLFW/glfw3.h"

struct Combat {
    float       hitPointsMax    {};
    float       hitPoints       {};
    float       meleeDamage     {};  // TODO: add min/max and randomly choose?
    double      attackStartedAt {};
    double      attackDuration  {};
    double      diedAt          {};
    uint32_t    attackFrame     {};
    LootTableID lootTableId     {};

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