#include "loot_table.h"
#include <dlb_types.h>
#include <stdlib.h>

static LootTable lootTables[(size_t)LootTableID::Count];

static void loot_table_add_drop(LootTableID lootTableId, ItemType itemType, uint32_t min, uint32_t max);

void loot_table_init(void)
{
    loot_table_add_drop(LootTableID::LT_Sam, ItemType::Currency, 1, 100000);
    loot_table_add_drop(LootTableID::LT_Slime, ItemType::Currency, 1, 4);
}

static void loot_table_add_drop(LootTableID lootTableId, ItemType itemType, uint32_t min, uint32_t max)
{
    LootTable &lootTable = lootTables[(size_t)lootTableId];
    LootDrop *drop = 0;
    for (int i = 0; i < ARRAY_SIZE(lootTable.drops); i++) {
        if (lootTable.drops[i].itemType == ItemType::Empty) {
            drop = &lootTable.drops[i];
            break;
        }
    }
    assert(drop);  // outta space yo

    if (drop) {
        drop->itemType = itemType;
        drop->minCount = min;
        drop->maxCount = max;
    }
}

uint32_t loot_table_roll_coins(LootTableID lootTableId, int monster_lvl)
{
    LootTable &table = lootTables[(size_t)lootTableId];
    uint32_t coins = 0;
    for (uint32_t i = 0; i < ARRAY_SIZE(table.drops); i++) {
        if (table.drops[i].itemType == ItemType::Currency) {
            LootDrop &drop = table.drops[i];
            coins += dlb_rand32u_range(drop.minCount, drop.maxCount) * monster_lvl;
        }
    }
    return coins;
}

void loot_table_roll_drops(LootTableID lootTableId)
{
    UNUSED(lootTableId);
}