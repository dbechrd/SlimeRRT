#pragma once
#include "item.h"

enum class LootTableID {
    LT_Sam,
    LT_Slime,
    //LT_2,
    //LT_3,
    //LT_4,
    //LT_5,
    //LT_6,
    //LT_7,
    //LT_8,
    //LT_9,
    Count
};

struct LootDrop {
    ItemType itemType;
    uint32_t minCount;
    uint32_t maxCount;
};

struct LootTable {
    LootTableID id;
    LootDrop    drops[16];
};

void loot_table_init(void);
uint32_t loot_table_roll_coins(LootTableID lootTableId);
void loot_table_roll_drops(LootTableID lootTableId);