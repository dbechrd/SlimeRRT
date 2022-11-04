#pragma once

#include "../helpers.h"

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