#pragma once
#include "item_world.h"

#define SV_WORLD_ITEMS_MAX     8 //4096
#define SV_WORLD_ITEM_LIFETIME 8 //600 // despawn items after 10 minutes

struct ItemSystem {
    ItemSystem(void);
    ~ItemSystem(void);

    void ItemSystem::SpawnItem(Vector3 pos, Catalog::ItemID id);
    size_t ItemsActive(void);
    void Update(double dt);
    void PushAll(DrawList& drawList);

private:
    ItemWorld *Alloc(void);

    ItemWorld  items[SV_WORLD_ITEMS_MAX]{};
    ItemWorld *itemsFree{};
    size_t     itemsActiveCount{};
};