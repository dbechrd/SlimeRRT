#pragma once

#include "../helpers.h"

// TODO: ParticleSystems should also be attached to an EntityID, so that when
// and entity is despawned, the particles don't keep playing on top of an
// invisible entity (this also makes effectCallbacks easier since they don't
// need to worry about other facets returning NULL).
enum FacetType {
    Facet_Attach,
    Facet_Body3D,
    Facet_Combat,
    Facet_Entity,
    Facet_Inventory,
    Facet_Sprite,
    Facet_Count
};

const char *FacetTypeStr(FacetType type)
{
    switch (type) {
        case Facet_Attach   : return "Facet_Attach";
        case Facet_Body3D   : return "Facet_Body3D";
        case Facet_Combat   : return "Facet_Combat";
        case Facet_Entity   : return "Facet_Entity";
        case Facet_Inventory: return "Facet_Inventory";
        case Facet_Sprite   : return "Facet_Sprite";
        default: DLB_ASSERT(!"Unknown facet type"); return "<FACET_UKNOWN_TYPE>";
    }
}

struct Facet {
    EntityID  entityId;
    FacetType facetType;
};