#include "loot_table.h"
#include <dlb_types.h>
#include <stdlib.h>

LootSystem::LootSystem(void)
{
    AddDropToTable(LootTableID::LT_Sam, ItemType::Currency, 1, 100000);
    AddDropToTable(LootTableID::LT_Slime, ItemType::Currency, 1, 4);
}

void LootSystem::AddDropToTable(LootTableID lootTableId, ItemType itemType, uint32_t min, uint32_t max)
{
    LootTable &lootTable = lootTableRegistry[(size_t)lootTableId];
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

uint32_t LootSystem::RollCoins(LootTableID lootTableId, int monster_lvl)
{
    LootTable &table = lootTableRegistry[(size_t)lootTableId];
    uint32_t coins = 0;
    for (uint32_t i = 0; i < ARRAY_SIZE(table.drops); i++) {
        if (table.drops[i].itemType == ItemType::Currency) {
            LootDrop &drop = table.drops[i];
            coins += dlb_rand32u_range(drop.minCount, drop.maxCount) * monster_lvl;
        }
    }
    return coins;
}

void LootSystem::RollDrops(LootTableID lootTableId)
{
    UNUSED(lootTableId);
}