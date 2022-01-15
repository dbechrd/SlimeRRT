#pragma once
#include "loot_table.h"

struct Combat {
    int         hitPointsMax    {};
    int         hitPoints       {};
    int         meleeDamage     {};  // TODO: add min/max and randomly choose?
    double      attackStartedAt {};
    double      attackDuration  {};
    uint32_t    attackFrame     {};
    LootTableID lootTableId     {};
};