#pragma once
#include "helpers.h"
#include "world_item.h"
#include <unordered_map>
#include <vector>

// This manages items spawned into the world as physics bodies; see Catalog::ItemDatabase for the actual item data
struct ItemSystem {
    ItemSystem  (void) { worldItems.reserve(SV_MAX_ITEMS); }
    ~ItemSystem (void) {}

    WorldItem *SpawnItem           (Vector3 pos, ItemUID itemUid, uint32_t count, EntityUID euid = 0);
    WorldItem *Find                (EntityUID eid);
    ErrorType  Remove              (EntityUID eid);
    void       Update              (double dt);
    void       DespawnDeadEntities (double despawnDelay = 0);
    void       PushAll             (DrawList& drawList);

    std::vector<WorldItem> worldItems{};
    std::unordered_map<EntityUID, uint32_t> byEuid{};  // map of world item entity id -> items[] index

private:
    const char *LOG_SRC = "ItemSystem";
};