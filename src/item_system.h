#pragma once
#include "helpers.h"
#include "item_world.h"

struct ItemSystem {
    ItemSystem(void) {};
    ~ItemSystem(void) {};

    ItemWorld *SpawnItem(Vector3 pos, Catalog::ItemID catalogId, uint32_t stackCount, uint32_t id = 0);
    ItemWorld *Find(uint32_t id);
    bool Remove(uint32_t itemId);
    void Update(double dt);
    void DespawnDeadEntities(void);
    void CL_DespawnStaleEntities(void);
    void PushAll(DrawList& drawList);

    // NOTE: DOn't modify these from outside.. need to figure out how to make world.cpp less of a mess
    // Probably would help to unify entities in some way so there's less duplication in e.g. CL_IntepolateBody
    ItemWorld items[SV_MAX_ITEMS]{};
    size_t    itemsCount{};

private:
    ItemWorld *Alloc(void);
};