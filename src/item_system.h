#pragma once
#include "helpers.h"
#include "item_world.h"
#include <unordered_map>
#include <vector>

// This manages items spawned into the world as physics bodies; see Catalog::ItemDatabase for the actual item data
struct ItemSystem {
    ItemSystem  (void) { worldItems.reserve(SV_MAX_ITEMS); };
    ~ItemSystem (void) {};

    ItemWorld *SpawnItem           (Vector3 pos, ItemUID itemUid, uint32_t count, EntityUID euid = 0);
    ItemWorld *Find                (EntityUID eid);
    bool       Remove              (EntityUID eid);
    void       Update              (double dt);
    void       DespawnDeadEntities (double pickupDespawnDelay = 0);
    void       PushAll             (DrawList& drawList);

    std::vector<ItemWorld> worldItems{};
    std::unordered_map<EntityUID, uint32_t> byEuid{};  // map of world item entity id -> items[] index
};