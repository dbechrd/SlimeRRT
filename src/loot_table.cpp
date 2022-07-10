#include "loot_table.h"
#include "catalog/items.h"
#include <dlb_types.h>
#include <stdlib.h>

LootSystem::LootSystem(void)
{
    InitLootTable(LootTableID::LT_Sam, 1);
    AddDropToTable(LootTableID::LT_Sam,   ItemClass_Currency, 1, 4, 1.0f);

    InitLootTable(LootTableID::LT_Slime, 3);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Currency, 1, 3, 0.85f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Weapon,   1, 1, 0.20f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Shield,   1, 1, 0.20f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Ring,     1, 1, 0.05f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Amulet,   1, 1, 0.05f);
}

void LootSystem::InitLootTable(LootTableID lootTableId, uint8_t maxDrops)
{
    LootTable &lootTable = lootTableRegistry[(size_t)lootTableId];
    lootTable.maxDrops = maxDrops;
}

void LootSystem::AddDropToTable(LootTableID lootTableId, ItemClass itemClass, uint32_t min, uint32_t max, float pctChance)
{
    LootTable &lootTable = lootTableRegistry[(size_t)lootTableId];
    LootDrop *drop = 0;
    for (int i = 0; i < ARRAY_SIZE(lootTable.drops); i++) {
        if (lootTable.drops[i].maxCount == 0) {
            drop = &lootTable.drops[i];
            break;
        }
    }
    assert(drop);  // outta space yo

    if (drop) {
        drop->itemClass = itemClass;
        drop->minCount = min;
        drop->maxCount = max;
        drop->pctChance = pctChance;
    }
}

uint32_t LootSystem::RollCoins(LootTableID lootTableId, int monster_lvl)
{
    LootTable &table = lootTableRegistry[(size_t)lootTableId];
    uint32_t coins = 0;
    for (uint32_t i = 0; i < ARRAY_SIZE(table.drops); i++) {
        LootDrop &drop = table.drops[i];
        if (drop.itemClass == ItemClass_Currency) {
            coins += dlb_rand32u_range(drop.minCount, drop.maxCount) * monster_lvl;
        }
    }
    return coins;
}

void LootSystem::RollDrops(LootTableID lootTableId, std::function<void(ItemStack &itemStack)> callback)
{
    LootTable &table = lootTableRegistry[(size_t)lootTableId];
    uint8_t dropCount = 0;
    for (uint32_t i = 0; dropCount < table.maxDrops && i < ARRAY_SIZE(table.drops); i++) {
        LootDrop &drop = table.drops[i];
        if (drop.maxCount && dlb_rand32f() < drop.pctChance) {
            ItemStack dropStack{};

            // TODO: Find a random item with the right ilvl range and item class. This probably means
            // that combat.lootTableId should instead be calculated dynamically based on the monster
            // level, area level, etc.
            // HACK: Find first item that has the class we want to drop
            for (ItemType itemType = 0; itemType < ItemType_Count; itemType++) {
                const Catalog::Item &item = Catalog::g_items.Find(itemType);
                if (item.itemClass == drop.itemClass) {
                    dropStack.itemType = itemType;
                    break;
                }
            }
            dropStack.count = dlb_rand32u_range(drop.minCount, drop.maxCount);
            callback(dropStack);
            dropCount++;
        }
    }
}