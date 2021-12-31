#include "item_system.h"
#include "raylib/raylib.h"

ItemSystem::ItemSystem(void)
{
    // Initialze intrusive free lists
    for (size_t i = 0; i < SV_WORLD_ITEMS_MAX; i++) {
        if (i < SV_WORLD_ITEMS_MAX - 1) {
            items[i].next = &items[i + 1];
        }
    }
    itemsFree = items;
}

ItemSystem::~ItemSystem(void)
{
    //memset(items, 0, sizeof(items));
    //itemsFree = 0;
}

void ItemSystem::SpawnItem(Vector3 pos, Catalog::ItemID id)
{
    ItemWorld *itemPtr = Alloc();
    if (!itemPtr) {
        return;
    }

    ItemWorld &item = *itemPtr;
    item.stack.id = id;
    item.stack.stackCount = 1;
    item.body.position = pos;
    static uint8_t itemIdx = 0;
    itemIdx = (itemIdx + 1) % 10;
    uint8_t grayWeight = (uint8_t)(255 / 10 * itemIdx);
    item.color = { grayWeight, grayWeight, grayWeight, 255 };
    item.spawnedAt = GetTime();
}

ItemWorld *ItemSystem::Alloc(void)
{
    // Allocate effect
    ItemWorld* item = itemsFree;
    if (!item) {
        // TODO: Delete oldest item instead of dropping new one
        TraceLog(LOG_ERROR, "Item pool is full; discarding item.");
        return 0;
    }
    itemsFree = item->next;
    itemsActiveCount++;
    return item;
}

size_t ItemSystem::ItemsActive(void)
{
    return itemsActiveCount;
}

void ItemSystem::Update(double dt)
{
    assert(itemsActiveCount <= SV_WORLD_ITEMS_MAX);

    size_t itemsCounted = 0;
    for (size_t i = 0; itemsCounted < itemsActiveCount; i++) {
        ItemWorld& item = items[i];
        if (item.stack.id == Catalog::ItemID::Empty)
            continue;

        if (GetTime() - item.spawnedAt < SV_WORLD_ITEM_LIFETIME) {
            item.body.Update(dt);
            // TODO: do we need animated sprites for world items?
            //sprite_update(item.sprite, dt);
        } else {
            // Return item to free list
            item = {};
            item.next = itemsFree;
            itemsFree = &item;
            assert(itemsActiveCount > 0);
            itemsActiveCount--;
        }
        itemsCounted++;
    }
}

void ItemSystem::PushAll(DrawList& drawList)
{
    assert(itemsActiveCount <= SV_WORLD_ITEMS_MAX);

    size_t itemsPushed = 0;
    for (size_t i = 0; itemsPushed < itemsActiveCount; i++) {
        const ItemWorld &item = items[i];
        if (item.stack.id == Catalog::ItemID::Empty)
            continue;

        drawList.Push(item);
        itemsPushed++;
    }
}