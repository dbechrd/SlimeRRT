#pragma once
#include "facet.h"
#include "../loot_table.h"

struct Combat : public Facet {
    typedef uint8_t Flags;
    enum : Flags {
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
    uint32_t    xp               {};  // How much xp entity has *and* how much you get when you kill it (might get scaled by penalties/bonuses)
    LootTableID lootTableId      {};

    double      attackStartedAt  {};
    double      attackDuration   {};
    double      diedAt           {};
    bool        droppedHitLoot   {};  // loot on hit? could be fun for some slime ball items to fly off when attacking
    bool        droppedDeathLoot {};  // loot on death, proper loot roll

    float   TakeDamage (float damage);
    uint8_t GrantXP    (uint32_t xpToGrant);  // Returns number of levels the entity gained due to the xp (or 0)
    void    Update     (double dt);
};
