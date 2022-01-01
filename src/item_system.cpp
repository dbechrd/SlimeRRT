#include "item_system.h"
#include "raylib/raylib.h"

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
    item.body.friction = 0.1f;

    static const SpriteDef *coinSpriteDef{};
    if (!coinSpriteDef) {
        const Spritesheet &coinSpritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Coin);
        coinSpriteDef = coinSpritesheet.FindSprite("coin");
        //assert(coinSpriteDef);
    }
    item.sprite.spriteDef = coinSpriteDef;
    item.sprite.scale = 1.0f;
    item.spawnedAt = GetTime();
}

ItemWorld *ItemSystem::Alloc(void)
{
    if (itemsCount == SV_MAX_WORLD_ITEMS) {
        // TODO: Delete oldest item instead of dropping new one
        TraceLog(LOG_ERROR, "Item pool is full; discarding item.");
        return 0;
    }
    ItemWorld *item = &items[itemsCount];
    itemsCount++;
    return item;
}

size_t ItemSystem::ItemsActive(void) const
{
    return itemsCount;
}

ItemWorld *ItemSystem::Items(void)
{
    return items;
}

void ItemSystem::Update(double dt)
{
    assert(itemsCount <= SV_MAX_WORLD_ITEMS);

    size_t i = 0;
    while (i < itemsCount) {
        ItemWorld& item = items[i];
        assert(item.stack.id != Catalog::ItemID::Empty);

        if (item.pickedUp || (GetTime() - item.spawnedAt >= SV_WORLD_ITEM_LIFETIME)) {
            // Return item to free list
            assert(itemsCount > 0);
            itemsCount--;

            // Copy last item to empty slot to keep densely packed
            if (itemsCount) {
                item = items[itemsCount];
            }
            items[itemsCount] = {};
        } else {
            item.body.Update(dt);
            sprite_update(item.sprite, dt);
            // TODO: do we need animated sprites for world items?
            //sprite_update(item.sprite, dt);
            i++;
        }
    }
}

void ItemSystem::PushAll(DrawList& drawList)
{
    assert(itemsCount <= SV_MAX_WORLD_ITEMS);

    size_t itemsPushed = 0;
    for (size_t i = 0; itemsPushed < itemsCount; i++) {
        const ItemWorld &item = items[i];
        assert(item.stack.id != Catalog::ItemID::Empty);
        drawList.Push(item);
        itemsPushed++;
    }
}