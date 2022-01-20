#pragma once
#include "helpers.h"
#include "item_world.h"
#include <unordered_map>
#include <vector>

struct ItemSystem {
    ItemSystem(void) { items.reserve(SV_MAX_ITEMS); };
    ~ItemSystem(void) {};

    ItemWorld *SpawnItem(Vector3 pos, Catalog::ItemID catalogId, uint32_t stackCount, uint32_t id = 0);
    const std::vector<ItemWorld> &ItemSystem::Items() const;
    ItemWorld *Find(uint32_t id);
    bool Remove(uint32_t itemId);
    void Update(double dt);
    void DespawnDeadEntities(double pickupDespawnDelay = 0);
    void PushAll(DrawList& drawList);

    std::vector<ItemWorld> items{};
    std::unordered_map<uint32_t, size_t> byId{};  // map of item.id -> items[] index
};