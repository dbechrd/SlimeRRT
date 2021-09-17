#pragma once
#include "loot_table.h"

struct Combat {
    float maxHitPoints     {};
    float hitPoints        {};
    float meleeDamage      {};  // TODO: add min/max and randomly choose?
    double attackStartedAt {};
    double attackDuration  {};
    LootTable lootTable    {};
};