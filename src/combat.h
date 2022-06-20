#pragma once
#include "clock.h"
#include "loot_table.h"
#include "dlb_types.h"

struct Combat {
    uint8_t     level            {};
    float       hitPointsMax     {};
    float       hitPoints        {};
    float       meleeDamage      {};  // TODO: add min/max and randomly choose?
    uint32_t    xpMin            {};
    uint32_t    xpMax            {};
    LootTableID lootTableId      {};

    double      attackStartedAt  {};
    double      attackDuration   {};
    double      diedAt           {};
    bool        droppedHitLoot   {};  // loot on hit? could be fun for some slime items to fly off when attacking
    bool        droppedDeathLoot {};  // loot on death, proper loot roll

    inline float DealDamage(float damage)
    {
        const float dealt = CLAMP(damage, 0.0f, hitPoints);
        hitPoints -= dealt;
        if (!hitPoints) {
            diedAt = g_clock.now;
        }
        return dealt;
    }
};