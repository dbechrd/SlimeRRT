#pragma once
#include "helpers.h"
#include "item_world.h"

struct ItemSystem {
    ItemSystem(void) {};
    ~ItemSystem(void) {};

    void ItemSystem::SpawnItem(Vector3 pos, Catalog::ItemID id, uint32_t stackCount);
    size_t ItemsActive(void) const;
    ItemWorld *Items(void);
    void Update(double dt);
    void PushAll(DrawList& drawList);

private:
    ItemWorld *Alloc(void);

    ItemWorld  items[SV_MAX_WORLD_ITEMS]{};
    size_t     itemsCount{};
};