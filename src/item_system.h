#pragma once
#include "helpers.h"
#include "item_world.h"
#include <unordered_map>
#include <vector>

// This manages items spawned into the world as physics bodies; see Catalog::ItemDatabase for the actual item data
struct ItemSystem {
    ItemSystem(void) { items.reserve(SV_MAX_ITEMS); };
    ~ItemSystem(void) {};

    ItemWorld *SpawnItem(Vector3 pos, const ItemProto &proto, uint32_t count, uint32_t uid = 0);
    ItemWorld *Find(uint32_t uid);
    bool Remove(uint32_t uid);
    void Update(double dt);
    void DespawnDeadEntities(double pickupDespawnDelay = 0);
    void PushAll(DrawList& drawList);

    std::vector<ItemWorld> items{};
    std::unordered_map<uint32_t, uint32_t> byUid{};  // map of item.uid -> items[] index
};