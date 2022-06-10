#pragma once
#include "catalog/items.h"

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
    ItemClass itemClass {};
    uint32_t  minCount  {};
    uint32_t  maxCount  {};
};

struct LootTable {
    LootTableID id    {};
    LootDrop    drops [16]{};
};

struct LootSystem {
    LootSystem(void);
    uint32_t RollCoins  (LootTableID lootTableId, int monster_lvl);
    void     RollDrops  (LootTableID lootTableId);

private:
    void AddDropToTable(LootTableID lootTableId, ItemClass itemClass, uint32_t min, uint32_t max);

    LootTable lootTableRegistry[(size_t)LootTableID::Count]{};
};
