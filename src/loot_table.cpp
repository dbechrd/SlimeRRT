#include "loot_table.h"
#include "catalog/items.h"
#include <dlb_types.h>
#include <stdlib.h>

LootSystem::LootSystem(void)
{
    InitLootTable(LootTableID::LT_Sam, 1);
    AddDropToTable(LootTableID::LT_Sam,   ItemClass_Currency, 1, 4, 1.0f);

    InitLootTable(LootTableID::LT_Slime, 3);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Empty,    1, 1, 0.50f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Currency, 1, 3, 0.30f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Weapon,   1, 1, 0.05f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Armor,    1, 1, 0.10f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Ring,     1, 1, 0.025f);
    AddDropToTable(LootTableID::LT_Slime, ItemClass_Amulet,   1, 1, 0.025f);

    for (int i = 0; i < 1000; i++) {
        RollDrops(LootTableID::LT_Slime, [&](ItemStack dropStack) {});
    }

    printf("Roll results:\n"
        "  Empty    %3d (%.03f)\n"
        "  Currency %3d (%.03f)\n"
        "  Weapon   %3d (%.03f)\n"
        "  Armor    %3d (%.03f)\n"
        "  Ring     %3d (%.03f)\n"
        "  Amulet   %3d (%.03f)\n",
        rollsPerClass[ItemClass_Empty], rollsPerClass[ItemClass_Empty] / 3000.0f,
        rollsPerClass[ItemClass_Currency], rollsPerClass[ItemClass_Currency] / 3000.0f,
        rollsPerClass[ItemClass_Weapon], rollsPerClass[ItemClass_Weapon] / 3000.0f,
        rollsPerClass[ItemClass_Armor], rollsPerClass[ItemClass_Armor] / 3000.0f,
        rollsPerClass[ItemClass_Ring], rollsPerClass[ItemClass_Ring] / 3000.0f,
        rollsPerClass[ItemClass_Amulet], rollsPerClass[ItemClass_Amulet] / 3000.0f
    );

    Validate();
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

void LootSystem::Validate(void)
{
    for (int tableId = 1; tableId < (int)LootTableID::Count; tableId++) {
        LootTable &table = lootTableRegistry[(size_t)tableId];
        float pctTotal = 0;
        for (int i = 0; i < ARRAY_SIZE(table.drops); i++) {
            LootDrop &drop = table.drops[i];
            if (drop.maxCount) {
                pctTotal += drop.pctChance;
            }
        }

        // Ensure that the sum of the drop chances adds up to 1.0.
        //
        // NOTE(dlb): We could technically allow it to be less than one and assume
        // the rest of the chance is taken up by implicit NoDrops, but I think I'd
        // prefer to keep that explicit. I may change my mind later.
        float error = fabsf(1.0f - pctTotal);
        DLB_ASSERT(error < 0.001f);
    }
}

uint32_t LootSystem::RollCoins(LootTableID lootTableId, int monster_lvl)
{
    LootTable &table = lootTableRegistry[(size_t)lootTableId];
    uint32_t coins = 0;
    for (int i = 0; i < ARRAY_SIZE(table.drops); i++) {
        LootDrop &drop = table.drops[i];
        if (drop.itemClass == ItemClass_Currency) {
            coins += dlb_rand32u_range(drop.minCount, drop.maxCount) * monster_lvl;
        }
    }
    return coins;
}

void LootSystem::RollDrops(LootTableID lootTableId, std::function<void(ItemStack dropStack)> callback)
{
    if (lootTableId == LootTableID::LT_Empty) {
        return;
    }

    LootTable &table = lootTableRegistry[(size_t)lootTableId];

    for (int roll = 0; roll < table.maxDrops; roll++) {
        // Roll 0-1 value
        const float rollResult = dlb_rand32f();

        // Check which item it matches in the drop table by keeping a running tally of chances, which
        // have to add up to 1.0 for this to work.
        float pctChance = 0;
        for (int i = 0; i < ARRAY_SIZE(table.drops); i++) {
            LootDrop &drop = table.drops[i];
            pctChance += drop.pctChance;
            if (rollResult < pctChance) {
                rollsPerClass[drop.itemClass]++;

                // If we roll a NoDrop, don't try to search for an item or spawn anything
                if (drop.itemClass) {
                    // TODO: Find a random item with the right ilvl range and item class. This probably means
                    // that combat.lootTableId should instead be calculated dynamically based on the monster
                    // level, area level, etc.
                    // HACK: Find first item that has the class we want to drop
                    ItemStack dropStack{};
                    for (ItemType itemType = 0; itemType < ItemType_Count; itemType++) {
                        const ItemProto &proto = g_item_catalog.FindProto(itemType);
                        if (proto.itemClass == drop.itemClass) {
                            dropStack.uid = g_item_db.Spawn(itemType);
                            dropStack.count = dlb_rand32u_range(drop.minCount, drop.maxCount);
                            break;
                        }
                    }
                    DLB_ASSERT(dropStack.uid);
                    DLB_ASSERT(dropStack.count);
                    if (dropStack.uid && dropStack.count) {
                        callback(dropStack);
                    }
                }
                break;
            }
        }
    }
}