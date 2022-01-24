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
    if (items.size() == SV_MAX_ITEMS) {
        // TODO: Delete oldest item instead of dropping new one
        TraceLog(LOG_ERROR, "Item pool is full; discarding item.");
        return 0;
    }
    ItemWorld item{};

    if (id) {
        item.id = id;
    } else {
        static uint32_t nextId = 0;
        nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
        item.id = nextId;
    }

    ItemWorld *existingItemWithId = Find(item.id);
    if (existingItemWithId) {
        TraceLog(LOG_ERROR, "Trying to spawn an item with id of an already existing item.");
        return 0;
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

    byId[item.id] = (uint32_t)items.size();
    return &items.emplace_back(item);
}

ItemWorld *ItemSystem::Find(uint32_t itemId)
{
    if (!itemId) {
        return 0;
    }

    auto elem = byId.find(itemId);
    if (elem == byId.end()) {
        return 0;
    }

    size_t idx = elem->second;
    assert(idx < items.size());
    return &items[idx];
}

bool ItemSystem::Remove(uint32_t itemId)
{
    TraceLog(LOG_DEBUG, "Despawning item %u", itemId);

    auto elem = byId.find(itemId);
    if (elem == byId.end()) {
        TraceLog(LOG_WARNING, "Cannot remove an item that doesn't exist. itemId: %u", itemId);
        return false;
    }

    bool success = false;
    uint32_t idx = elem->second;
    uint32_t len = (uint32_t)items.size();
    if (idx < len) {
        if (idx == len - 1) {
            items.pop_back();
        } else {
            // Copy last item to empty slot to keep densely packed
            items[idx] = items.back();
            // Update hash table
            byId[items[idx].id] = idx;
            // Zero free slot
            items[items.size() - 1] = {};
            // Update size
            items.pop_back();
        }
        success = true;
    } else {
        TraceLog(LOG_ERROR, "Item index out of range. itemId: %u idx: %zu size: %zu", itemId, idx, items.size());
    }
    byId.erase(itemId);

    TraceLog(LOG_DEBUG, "Despawned item %u", itemId);
    return success;
}

void ItemSystem::Update(double dt)
{
    for (ItemWorld &item : items) {
        assert(item.stack.id != Catalog::ItemID::Empty);

        item.body.Update(dt);
        sprite_update(item.sprite, dt);
        // TODO: do we need animated sprites for world items?
        //sprite_update(item.sprite, dt);

        item.Update(dt);
    }
}

void ItemSystem::DespawnDeadEntities(double pickupDespawnDelay)
{
    const double now = glfwGetTime();
#if 1
    size_t i = 0;
    while (i < items.size()) {
        ItemWorld &item = items[i];
        assert(item.stack.id != Catalog::ItemID::Empty);

        // NOTE: Server adds extra pickupDespawnDelay to ensure all clients receive a snapshot
        // containing the pickup flag before despawning the item. This may not be necessary
        // one nearby_events are implemented and send item pickup notifications.
        if ((item.pickedUpAt && ((now - item.pickedUpAt) > pickupDespawnDelay)) ||
            (item.spawnedAt && ((now - item.spawnedAt) > SV_WORLD_ITEM_LIFETIME)))
        {
            // NOTE: If remove succeeds, don't increment index, next element to check is in the
            // same slot now.
            i += !Remove(item.id);
        } else {
            i++;
        }
    }
#else
    items.erase(std::remove_if(items.begin(), items.end(), [pickupDespawnDelay](const ItemWorld &item) {
        assert(item.stack.id != Catalog::ItemID::Empty);

        // NOTE: Server adds extra pickupDespawnDelay to ensure all clients receive a snapshot
        // containing the pickup flag before despawning the item. This may not be necessary
        // one nearby_events are implemented and send item pickup notifications.
        return ((item.pickedUpAt && ((now - item.pickedUpAt) > pickupDespawnDelay)) ||
                (item.spawnedAt && ((now - item.spawnedAt) > SV_WORLD_ITEM_LIFETIME)));
    }));
#endif
}

void ItemSystem::PushAll(DrawList &drawList)
{
    for (const ItemWorld &item : items) {
        if (item.stack.id != Catalog::ItemID::Empty) {
            drawList.Push(item);
        } else {
            assert(!"Empty item id");
        }
    }
}