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

ItemWorld *ItemSystem::SpawnItem(Vector3 pos, ItemUID itemUid, uint32_t count, EntityUID euid)
{
    if (!count) {
        TraceLog(LOG_WARNING, "Not spawning item stack with count 0.");
        return 0;
    }

    if (worldItems.size() == SV_MAX_ITEMS) {
        // TODO: Delete oldest item instead of discarding the new one
        TraceLog(LOG_ERROR, "Item pool is full; discarding item.");
        return 0;
    }
    ItemWorld worldItem{};

    if (euid) {
        ItemWorld *existingItemWithId = Find(euid);
        if (existingItemWithId) {
            TraceLog(LOG_ERROR, "Trying to spawn a world item that already exists in the world.");
            return 0;
        }
    } else {
        thread_local uint32_t nextUid = 0;
        euid = MAX(1, nextUid + 1); // Prevent ID zero from being used on overflow
    }

    uint32_t stackLimit = g_item_db.Find(itemUid).Proto().stackLimit;
    DLB_ASSERT(count <= stackLimit);
    worldItem.stack.uid = itemUid;
    worldItem.stack.count = MIN(count, stackLimit);  // Safe in release mode when assert is disabled

    Vector3 itemPos = pos;
    itemPos.x += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
    itemPos.y += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
    worldItem.body.Teleport(itemPos);
    float randX = dlb_rand32f_variance(METERS_TO_PIXELS(2.0f));
    float randY = dlb_rand32f_variance(METERS_TO_PIXELS(2.0f));
    float randZ = dlb_rand32f_range(3.0f, METERS_TO_PIXELS(4.0f));
    worldItem.body.velocity = { randX, randY, randZ };
    worldItem.body.restitution = 0.4f;
    worldItem.body.friction = 0.2f;

    Catalog::SpritesheetID spritesheetId = Catalog::SpritesheetID::Empty;
    //switch (uid) {
    //    case ItemID_Currency_Copper: spritesheetId = Catalog::SpritesheetID::Coin_Copper; break;
    //    case ItemID_Currency_Silver: spritesheetId = Catalog::SpritesheetID::Coin_Silver; break;
    //    case ItemID_Currency_Gilded: spritesheetId = Catalog::SpritesheetID::Coin_Gilded; break;
    //}
    if (spritesheetId != Catalog::SpritesheetID::Empty) {
        const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(spritesheetId);
        worldItem.sprite.spriteDef = spritesheet.FindSprite("coin");
    }
    worldItem.sprite.scale = 1.0f;
    worldItem.spawnedAt = g_clock.now;

    byEuid[worldItem.euid] = (uint32_t)worldItems.size();
    return &worldItems.emplace_back(worldItem);
}

ItemWorld *ItemSystem::Find(EntityUID euid)
{
    if (!euid) {
        return 0;
    }

    auto elem = byEuid.find(euid);
    if (elem == byEuid.end()) {
        return 0;
    }

    size_t idx = elem->second;
    assert(idx < worldItems.size());
    return &worldItems[idx];
}

bool ItemSystem::Remove(EntityUID euid)
{
#if SV_DEBUG_WORLD_ITEMS
    TraceLog(LOG_DEBUG, "Despawning item %u", euid);
#endif
    auto elem = byEuid.find(euid);
    if (elem == byEuid.end()) {
        //TraceLog(LOG_WARNING, "Cannot remove an item that doesn't exist. uid: %u", uid);
        return false;
    }

    bool success = false;
    uint32_t idx = elem->second;
    uint32_t len = (uint32_t)worldItems.size();
    if (idx < len) {
        if (idx == len - 1) {
            worldItems.pop_back();
        } else {
            // Copy last item to empty slot to keep densely packed
            worldItems[idx] = worldItems.back();
            // Update hash table
            byEuid[worldItems[idx].euid] = idx;
            // Zero free slot
            worldItems[worldItems.size() - 1] = {};
            // Update size
            worldItems.pop_back();
        }
        success = true;
    } else {
        TraceLog(LOG_ERROR, "Item index out of range. uid: %u idx: %zu size: %zu", euid, idx, worldItems.size());
    }
    byEuid.erase(euid);

#if SV_DEBUG_WORLD_ITEMS
    TraceLog(LOG_DEBUG, "Despawned item %u", uid);
#endif
    return success;
}

void ItemSystem::Update(double dt)
{
    for (ItemWorld &item : worldItems) {
        if (!item.stack.count) {
            continue;
        }
        DLB_ASSERT(item.stack.uid);

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
    while (i < worldItems.size()) {
        ItemWorld &item = worldItems[i];

        // NOTE: Server adds extra pickupDespawnDelay to ensure all clients receive a snapshot
        // containing the pickup flag before despawning the item. This may not be necessary
        // once nearby_events are implemented and send item pickup notifications.
        if (item.stack.count && (
            (item.pickedUpAt && ((g_clock.now - item.pickedUpAt) > pickupDespawnDelay)) ||
            (item.spawnedAt && ((g_clock.now - item.spawnedAt) > SV_WORLD_ITEM_LIFETIME))
        )) {
            DLB_ASSERT(item.stack.uid);
            // NOTE: If remove succeeds, don't increment index, next element to check is in the
            // same slot now.
            i += !Remove(item.euid);
        } else {
            i++;
        }
    }
}

void ItemSystem::PushAll(DrawList &drawList)
{
    for (ItemWorld &item : worldItems) {
        if (item.stack.count) {
            DLB_ASSERT(item.stack.uid);
            drawList.Push(item);
        }
    }
}