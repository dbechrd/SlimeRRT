#pragma once
#include "helpers.h"
#include "item_world.h"

struct ItemSystem {
    ItemSystem(void) {};
    ~ItemSystem(void) {};

    ItemWorld *SpawnItem(Vector3 pos, Catalog::ItemID catalogId, uint32_t stackCount, uint32_t id = 0);
    size_t ItemsActive(void) const;
    ItemWorld *At(size_t index);
    ItemWorld *Find(uint32_t id);
    bool Remove(uint32_t itemId);
    void Update(double dt);
    void DespawnDeadEntities(void);
    void CL_DespawnStaleEntities(void);
    void PushAll(DrawList& drawList);

private:
    ItemWorld *Alloc(void);

    ItemWorld  items[SV_MAX_ITEMS]{};
    size_t     itemsCount{};
};