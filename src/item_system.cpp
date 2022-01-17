#include "item_system.h"
#include "raylib/raylib.h"

// Server spawns items into world
// Server broadcasts ItemDrop event
// Client creates proxy, updates physics
// Client sends ItemPickup event (request, server will respond yes/no)
// Server validates item is still on ground and is close enough for player to pick up
// Server adds item to player's inventory
// Server sends InventoryUpdate event
// Server broadcasts ItemPickup event

ItemWorld *ItemSystem::SpawnItem(Vector3 pos, Catalog::ItemID catalogId, uint32_t count, uint32_t id)
{
    ItemWorld *itemPtr = Alloc();
    if (!itemPtr) {
        return 0;
    }

    ItemWorld &item = *itemPtr;

    if (id) {
        item.id = id;
    } else {
        static uint32_t nextId = 0;
        nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
        item.id = nextId;
    }

    item.stack.id = catalogId;
    item.stack.count = count;
    item.body.Teleport(pos);
    float randX = dlb_rand32f_variance(METERS_TO_PIXELS(2.0f));
    float randY = dlb_rand32f_variance(METERS_TO_PIXELS(2.0f));
    float randZ = dlb_rand32f_range(3.0f, METERS_TO_PIXELS(4.0f));
    item.body.velocity = { randX, randY, randZ };
    item.body.restitution = 0.2f;
    item.body.friction = 0.2f;

    static const SpriteDef *coinSpriteDef{};
    if (!coinSpriteDef) {
        const Spritesheet &coinSpritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Coin);
        coinSpriteDef = coinSpritesheet.FindSprite("coin");
        //assert(coinSpriteDef);
    }
    item.sprite.spriteDef = coinSpriteDef;
    item.sprite.scale = 1.0f;
    item.spawnedAt = glfwGetTime();
    return itemPtr;
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

ItemWorld *ItemSystem::Find(uint32_t itemId)
{
    if (!itemId) {
        return 0;
    }

    for (ItemWorld &item : items) {
        if (item.id == itemId) {
            return &item;
        }
    }
    return 0;
}

bool ItemSystem::Remove(uint32_t itemId)
{
    ItemWorld *item = Find(itemId);
    if (!item) {
        TraceLog(LOG_ERROR, "Cannot remove a item that doesn't exist. itemId: %u", itemId);
        return false;
    }

    TraceLog(LOG_DEBUG, "Despawn item %u", itemId);

    // Return item to free list
    assert(itemsCount > 0);
    itemsCount--;

    // Copy last item to empty slot to keep densely packed
    if (itemsCount) {
        *item = items[itemsCount];
    }
    items[itemsCount] = {};
    return true;
}

void ItemSystem::Update(double dt)
{
    assert(itemsCount < SV_MAX_ITEMS);

    size_t i = 0;
    while (i < itemsCount) {
        ItemWorld& item = items[i];
        assert(item.stack.id != Catalog::ItemID::Empty);

        item.body.Update(dt);
        sprite_update(item.sprite, dt);
        // TODO: do we need animated sprites for world items?
        //sprite_update(item.sprite, dt);

        item.Update(dt);
        i++;
    }
}

void ItemSystem::DespawnDeadEntities(void)
{
    assert(itemsCount < SV_MAX_ITEMS);

    size_t i = 0;
    while (i < itemsCount) {
        ItemWorld &item = items[i];
        assert(item.stack.id != Catalog::ItemID::Empty);

        if ((item.pickedUpAt && ((glfwGetTime() - item.pickedUpAt) > 1.0 / SNAPSHOT_SEND_RATE)) ||
            (item.spawnedAt  && ((glfwGetTime() - item.spawnedAt ) > SV_WORLD_ITEM_LIFETIME  ))) {
            if (Remove(item.id)) {
                continue;
            }
        }
        i++;
    }
}

void ItemSystem::CL_DespawnStaleEntities(void)
{
    assert(itemsCount < SV_MAX_ITEMS);

    DespawnDeadEntities();

    size_t i = 0;
    while (i < itemsCount) {
        ItemWorld &item = items[i];
        assert(item.stack.id != Catalog::ItemID::Empty);

        if ((item.pickedUpAt) ||
            (item.spawnedAt && ((glfwGetTime() - item.spawnedAt) > SV_WORLD_ITEM_LIFETIME))) {
            if (Remove(item.id)) {
                continue;
            }
        }
        i++;
    }
}

void ItemSystem::PushAll(DrawList& drawList)
{
    for (size_t i = 0; i < itemsCount; i++) {
        const ItemWorld &item = items[i];
        if (item.stack.id != Catalog::ItemID::Empty) {
            drawList.Push(item);
        } else {
            assert(!"Empty item id");
        }
    }
}