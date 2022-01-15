#include "item_system.h"
#include "raylib/raylib.h"

// Server spawns items into world
// Server broadcasts ItemDrop event
// Client creates proxy, updates physics
// Client sends ItemPickup event
// Server validates item is still on ground and is close enough for player to pick up
// Server adds item to player's inventory
// Server sends InventoryUpdate event
// Server broadcasts ItemPickup event

void ItemSystem::SpawnItem(Vector3i pos, Catalog::ItemID id, uint32_t count)
{
    ItemWorld *itemPtr = Alloc();
    if (!itemPtr) {
        return;
    }

    ItemWorld &item = *itemPtr;
    item.stack.id = id;
    item.stack.count = count;
    item.body.position = pos;
    int randX = dlb_rand32i_variance(METERS_TO_UNITS(3));
    int randY = dlb_rand32i_variance(METERS_TO_UNITS(3));
    int randZ = dlb_rand32i_range(2, METERS_TO_UNITS(4));
    item.body.velocity = { randX, randY, randZ };
    item.body.restitution = 0.8f;
    item.body.friction = 0.1f;

    static const SpriteDef *coinSpriteDef{};
    if (!coinSpriteDef) {
        const Spritesheet &coinSpritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Coin);
        coinSpriteDef = coinSpritesheet.FindSprite("coin");
        //assert(coinSpriteDef);
    }
    item.sprite.spriteDef = coinSpriteDef;
    item.sprite.scale = 1;
    item.spawnedAt = GetTime();
}

ItemWorld *ItemSystem::Alloc(void)
{
    if (itemsCount == SV_MAX_ITEMS) {
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
    assert(itemsCount <= SV_MAX_ITEMS);

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
    assert(itemsCount <= SV_MAX_ITEMS);

    size_t itemsPushed = 0;
    for (size_t i = 0; itemsPushed < itemsCount; i++) {
        const ItemWorld &item = items[i];
        assert(item.stack.id != Catalog::ItemID::Empty);
        drawList.Push(item);
        itemsPushed++;
    }
}