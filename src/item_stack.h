#pragma once
#include "catalog/items.h"
#include <cstdint>

struct ItemStack {
    Catalog::ItemID id    {};
    uint32_t        count {};  // TODO: Check this is < itemCatalog[id].stackLimit
};