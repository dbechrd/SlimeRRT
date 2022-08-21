#pragma once
#include "catalog/items.h"
#include <functional>

struct ItemSystem;
struct ItemStack;

enum class LootTableID {
    LT_Empty,
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
    float     pctChance {};  // 0.0 - 1.0 (% chance for this drop to roll)
};

struct LootTable {
    LootTableID id        {};
    uint8_t     maxDrops  {};  // if we roll this many drops, stop rolling
    LootDrop    drops     [16]{};
};

struct LootSystem {
    LootSystem(void);
    uint32_t RollCoins  (LootTableID lootTableId, int monster_lvl);
    void     RollDrops  (LootTableID lootTableId, std::function<void(ItemStack &itemStack)> callback);

private:
    void InitLootTable(LootTableID lootTableId, uint8_t maxDrops);
    void AddDropToTable(LootTableID lootTableId, ItemClass itemClass, uint32_t min, uint32_t max, float pctChance);

    LootTable lootTableRegistry[(size_t)LootTableID::Count]{};
};
