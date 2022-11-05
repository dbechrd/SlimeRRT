#include "facet_depot.h"

ErrorType FacetDepot::FacetAlloc(EntityID entityId, FacetType type, Facet **result)
{
    DLB_ASSERT(type >= 0);
    DLB_ASSERT(type < Facet_Count);

    size_t poolIdx = poolIndexByEntityID[type][entityId];
    if (poolIdx) {
        E_ERROR_RETURN(ErrorType::AllocFailed_Duplicate, "Entity id %u already has facet of type %u!", entityId, type);
    }

    Facet *facet = 0;
    switch (type) {
        case Facet_Attach: {
            Facet *facet = &attachPool.emplace_back();
            poolIdx = attachPool.size() - 1;
            break;
        }
        case Facet_Body3D: {
            Facet *facet = &body3dPool.emplace_back();
            poolIdx = body3dPool.size() - 1;
            break;
        }
        case Facet_Combat: {
            Facet *facet = &combatPool.emplace_back();
            poolIdx = combatPool.size() - 1;
            break;
        }
        case Facet_Entity: {
            Facet *facet = &entityPool.emplace_back();
            poolIdx = entityPool.size() - 1;
            break;
        }
        case Facet_Inventory: {
            Facet *facet = &inventoryPool.emplace_back();
            poolIdx = inventoryPool.size() - 1;
            break;
        }
        case Facet_Sprite: {
            Facet *facet = &spritePool.emplace_back();
            poolIdx = spritePool.size() - 1;
            break;
        }
        default: {
            E_ERROR_RETURN(ErrorType::UnknownType, "Unrecognized facet type %u", type);
            break;
        }
    }
    DLB_ASSERT(facet);
    DLB_ASSERT(poolIdx);

    facet->entityId = entityId;
    facet->facetType = type;
    poolIndexByEntityID[type][entityId] = poolIdx;
    if (result) *result = facet;
    return ErrorType::Success;
}

Facet *FacetDepot::FacetFind(EntityID entityId, FacetType type)
{
    DLB_ASSERT(type >= 0);
    DLB_ASSERT(type < Facet_Count);

    size_t poolIdx = poolIndexByEntityID[type][entityId];
    if (poolIdx) {
        switch (type) {
            case Facet_Attach:    return &attachPool[poolIdx];
            case Facet_Body3D:    return &body3dPool[poolIdx];
            case Facet_Combat:    return &combatPool[poolIdx];
            case Facet_Entity:    return &entityPool[poolIdx];
            case Facet_Inventory: return &inventoryPool[poolIdx];
            case Facet_Sprite:    return &spritePool[poolIdx];
        }
    }
    return 0;
}

void FacetDepot::FacetFree(EntityID entityId, FacetType type)
{
    DLB_ASSERT(type >= 0);
    DLB_ASSERT(type < Facet_Count);

    size_t poolIdx = poolIndexByEntityID[type][entityId];
    if (!poolIdx) {
        return;
    }

    switch (type) {
        case Facet_Attach:
        {
            attachPool[poolIdx] = std::move(attachPool.back());
            attachPool.pop_back();
            break;
        }
        case Facet_Body3D:
        {
            body3dPool[poolIdx] = std::move(body3dPool.back());
            body3dPool.pop_back();
            break;
        }
        case Facet_Combat:
        {
            combatPool[poolIdx] = std::move(combatPool.back());
            combatPool.pop_back();
            break;
        }
        case Facet_Entity:
        {
            entityPool[poolIdx] = std::move(entityPool.back());
            entityPool.pop_back();
            break;
        }
        case Facet_Inventory:
        {
            inventoryPool[poolIdx] = std::move(inventoryPool.back());
            inventoryPool.pop_back();
            break;
        }
        case Facet_Sprite:
        {
            spritePool[poolIdx] = std::move(spritePool.back());
            spritePool.pop_back();
            break;
        }
    }
}

ErrorType FacetDepot::EntityAlloc(EntityID entityId, EntityType type, Entity **result)
{
    Entity *entity = 0;
    E_ERROR_RETURN(FacetAlloc(entityId, Facet_Entity, (Facet **)entity),
        "Failed to allocate entity id %u type %u", entityId, type);
    entityIdsByType[type].emplace_back(entityId);
    entity->entityType = type;
    if (result) *result = entity;
}

Entity *FacetDepot::EntityFind(EntityID entityId)
{
    Entity *entity = 0;
    size_t poolIdx = poolIndexByEntityID[Facet_Entity][entityId];
    if (poolIdx) {
        entity = &entityPool[poolIdx];
        DLB_ASSERT(entity->entityId == entityId);
    }
    return entity;
}

void FacetDepot::EntityFree(EntityID entityId)
{
    E_DEBUG("EntityFree [%u]", entityId);

    Entity *entity = EntityFind(entityId);
    if (entity) {
        auto &vec = entityIdsByType[entity->entityType];
        vec.erase(std::remove(vec.begin(), vec.end(), entityId), vec.end());
    } else {
        E_WARN("No entity facet found for id %u. Removing other facets anyway.", entityId);
    }

    for (int type = 0; type < Facet_Count; type++) {
        FacetFree(entityId, (FacetType)type);
    }
}
