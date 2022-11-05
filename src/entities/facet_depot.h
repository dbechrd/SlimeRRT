#pragma once
#include "attach.h"
#include "body.h"
#include "combat.h"
#include "entity.h"
#include "inventory.h"
#include "sprite.h"
#include <unordered_map>
#include <vector>

// TODO: Refactor WorldItems out of ItemSystem into EntityCollector
// TODO: ResourceCatalog byType/byId for shared resources (textures, sprites, etc.)
struct FacetDepot {
    // NOTE: Slot 0 is reserved for the EMPTY facet in each pool
    std::vector<Attach>    attachPool    {{}};
    std::vector<Body3D>    body3dPool    {{}};
    std::vector<Combat>    combatPool    {{}};
    std::vector<Entity>    entityPool    {{}};
    std::vector<Inventory> inventoryPool {{}};
    std::vector<Sprite>    spritePool    {{}};
    std::unordered_map<EntityID, size_t> poolIndexByEntityID[Facet_Count]{};
    std::vector<EntityID>entityIdsByType[Entity_Count]{};

    ErrorType  FacetAlloc  (EntityID entityId, FacetType type, Facet **result);
    Facet     *FacetFind   (EntityID entityId, FacetType type);
    void       FacetFree   (EntityID entityId, FacetType type);
    ErrorType  EntityAlloc (EntityID entityId, EntityType type, Entity **result);
    Entity    *EntityFind  (EntityID entityId);
    void       EntityFree  (EntityID entityId);

private:
    const char *LOG_SRC = "FacetDepot";
};