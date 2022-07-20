#include "item_system.h"
#include "catalog/spritesheets.h"
#include "raylib/raylib.h"
#include "sprite.h"
#include "dlb_rand.h"
#include "GLFW/glfw3.h"

// Server spawns items into world
// Server broadcasts ItemDrop event
// Client creates proxy, updates physics
// Client sends ItemPickup event (request, server will respond yes/no)
// Server validates item is still on ground and is close enough for player to pick up
// Server adds item to player's inventory
// Server sends InventoryUpdate event
// Server broadcasts ItemPickup event

ItemWorld *ItemSystem::SpawnItem(Vector3 pos, ItemType itemId, uint32_t count, uint32_t uid)
{
    assert(count >= 0);

    if (!count) {
        TraceLog(LOG_WARNING, "Not spawning item stack with count 0.");
        return 0;
    }

    if (items.size() == SV_MAX_ITEMS) {
        // TODO: Delete oldest item instead of discarding the new one
        TraceLog(LOG_ERROR, "Item pool is full; discarding item.");
        return 0;
    }
    ItemWorld item{};

    if (uid) {
        item.uid = uid;
    } else {
        thread_local uint32_t nextUid = 0;
        nextUid = MAX(1, nextUid + 1); // Prevent ID zero from being used on overflow
        item.uid = nextUid;
    }

    ItemWorld *existingItemWithId = Find(item.uid);
    if (existingItemWithId) {
        TraceLog(LOG_ERROR, "Trying to spawn an item with type of an already existing item.");
        return 0;
    }

    item.stack.itemType = itemId;
    item.stack.count = count;

    Vector3 itemPos = pos;
    itemPos.x += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
    itemPos.y += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
    item.body.Teleport(itemPos);
    float randX = dlb_rand32f_variance(METERS_TO_PIXELS(2.0f));
    float randY = dlb_rand32f_variance(METERS_TO_PIXELS(2.0f));
    float randZ = dlb_rand32f_range(3.0f, METERS_TO_PIXELS(4.0f));
    item.body.velocity = { randX, randY, randZ };
    item.body.restitution = 0.2f;
    item.body.friction = 0.2f;

    Catalog::SpritesheetID spritesheetId = Catalog::SpritesheetID::Empty;
    //switch (uid) {
    //    case ItemID_Currency_Copper: spritesheetId = Catalog::SpritesheetID::Coin_Copper; break;
    //    case ItemID_Currency_Silver: spritesheetId = Catalog::SpritesheetID::Coin_Silver; break;
    //    case ItemID_Currency_Gilded: spritesheetId = Catalog::SpritesheetID::Coin_Gilded; break;
    //}
    if (spritesheetId != Catalog::SpritesheetID::Empty) {
        const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(spritesheetId);
        item.sprite.spriteDef = spritesheet.FindSprite("coin");
    }
    item.sprite.scale = 1.0f;
    item.spawnedAt = g_clock.now;

    byUid[item.uid] = (uint32_t)items.size();
    return &items.emplace_back(item);
}

ItemWorld *ItemSystem::Find(uint32_t uid)
{
    if (!uid) {
        return 0;
    }

    auto elem = byUid.find(uid);
    if (elem == byUid.end()) {
        return 0;
    }

    size_t idx = elem->second;
    assert(idx < items.size());
    return &items[idx];
}

bool ItemSystem::Remove(uint32_t uid)
{
#if SV_DEBUG_WORLD_ITEMS
    TraceLog(LOG_DEBUG, "Despawning item %u", uid);
#endif
    auto elem = byUid.find(uid);
    if (elem == byUid.end()) {
        //TraceLog(LOG_WARNING, "Cannot remove an item that doesn't exist. uid: %u", uid);
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
            byUid[items[idx].uid] = idx;
            // Zero free slot
            items[items.size() - 1] = {};
            // Update size
            items.pop_back();
        }
        success = true;
    } else {
        TraceLog(LOG_ERROR, "Item index out of range. uid: %u idx: %zu size: %zu", uid, idx, items.size());
    }
    byUid.erase(uid);

#if SV_DEBUG_WORLD_ITEMS
    TraceLog(LOG_DEBUG, "Despawned item %u", uid);
#endif
    return success;
}

void ItemSystem::Update(double dt)
{
    for (ItemWorld &item : items) {
        if (!item.stack.itemType) {
            continue;
        }

        item.body.Update(dt);
        sprite_update(item.sprite, dt);
        // TODO: do we need animated sprites for world items?
        //sprite_update(item.sprite, dt);

        item.Update(dt);
    }
}

void ItemSystem::DespawnDeadEntities(double pickupDespawnDelay)
{
    size_t i = 0;
    while (i < items.size()) {
        ItemWorld &item = items[i];

        // NOTE: Server adds extra pickupDespawnDelay to ensure all clients receive a snapshot
        // containing the pickup flag before despawning the item. This may not be necessary
        // once nearby_events are implemented and send item pickup notifications.
        if (item.stack.itemType && (
            (item.pickedUpAt && ((g_clock.now - item.pickedUpAt) > pickupDespawnDelay)) ||
            (item.spawnedAt && ((g_clock.now - item.spawnedAt) > SV_WORLD_ITEM_LIFETIME))
        )) {
            // NOTE: If remove succeeds, don't increment index, next element to check is in the
            // same slot now.
            i += !Remove(item.uid);
        } else {
            i++;
        }
    }
}

void ItemSystem::PushAll(DrawList &drawList)
{
    for (const ItemWorld &item : items) {
        if (item.stack.itemType) {
            drawList.Push(item);
        }
    }
}