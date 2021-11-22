#pragma once
#include "loot_table.h"

struct Combat {
    float hitPointsMax      {};
    float hitPoints         {};
    float meleeDamage       {};  // TODO: add min/max and randomly choose?
    double attackStartedAt  {};
    double attackDuration   {};
    LootTableID lootTableId {};
};