#include "loot_table.h"
#include <stdlib.h>

LootTable lootTables[(size_t)LootTableID::Count];

static LootDrop *g_lootDrops;

void loot_table_init(void)
{
    if (g_lootDrops) {
        free(g_lootDrops);
    }

    g_lootDrops = (LootDrop *)calloc(16, sizeof(*g_lootDrops));
    LootDrop *next = g_lootDrops;

    for (LootTable &table : lootTables) {
        table.dropListCount = 1;
        table.dropList = next;
        next += table.dropListCount;
    }

    LootDrop &dropSam = lootTables[(size_t)LootTableID::LT_Sam].dropList[0];
    LootDrop &dropSlime = lootTables[(size_t)LootTableID::LT_Slime].dropList[0];

    dropSam.itemType = ItemType::Currency;
    dropSam.minCount = 1;
    dropSam.maxCount = 100000;

    dropSlime.itemType = ItemType::Currency;
    dropSlime.minCount = 1;
    dropSlime.maxCount = 4;
}

void loot_table_free(void)
{
    for (auto lootTable : lootTables) {
        lootTable.dropListCount = 0;
        lootTable.dropList = nullptr;
    }
    free(g_lootDrops);
}

uint32_t loot_table_roll_coins(LootTableID lootTableId, int monster_lvl)
{
    uint32_t coins = 0;
    
    LootTable &table = lootTables[(size_t)lootTableId];
    for (uint32_t i = 0; i < table.dropListCount; i++) {
        if (table.dropList[i].itemType == ItemType::Currency) {
            LootDrop &drop = table.dropList[i];
            coins += dlb_rand32u_range(drop.minCount, drop.maxCount) * monster_lvl;
        }
    }

    return coins;
}

void loot_table_roll_drops(LootTableID lootTableId)
{
    UNUSED(lootTableId);
}