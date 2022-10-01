#pragma once
#include "clock.h"
#include "loot_table.h"
#include "dlb_types.h"

struct Combat {
    enum Flags {
        Flag_None         = 0x0,
        Flag_TooBigToFail = 0x1,   // Invulerability
        Flag_Unused1      = 0x2,
        Flag_Unused2      = 0x4,
        Flag_Unused3      = 0x8,
        Flag_Unused4      = 0x10,
        Flag_Unused5      = 0x20,
        Flag_Unused6      = 0x40,
        Flag_Unused7      = 0x80,
    };

    Flags       flags            {};
    uint8_t     level            {};
    float       hitPointsMax     {};
    float       hitPointsPrev    {};
    float       hitPoints        {};
    float       hitPointsSmooth  {};  // For animating delta hp (damage/healing)
    float       meleeDamage      {};  // TODO: add min/max and randomly choose?
    uint32_t    xpMin            {};
    uint32_t    xpMax            {};
    LootTableID lootTableId      {};

    double      attackStartedAt  {};
    double      attackDuration   {};
    double      diedAt           {};
    bool        droppedHitLoot   {};  // loot on hit? could be fun for some slime ball items to fly off when attacking
    bool        droppedDeathLoot {};  // loot on death, proper loot roll

    inline float TakeDamage(float damage) {
        if (flags & Flag_TooBigToFail) {
            return 0;
        }

        hitPointsPrev = hitPoints;
        const float dealt = CLAMP(damage, 0.0f, hitPoints);
        hitPoints -= dealt;
        if (!hitPoints) {
            diedAt = g_clock.now;
        }
        return dealt;
    }

    void Update(double dt)
    {
        hitPointsSmooth += (hitPoints - hitPointsSmooth) * CLAMP(5.0f * (float)dt, 0.05f, 1.0f);
    }
};