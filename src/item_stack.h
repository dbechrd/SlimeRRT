#pragma once
#include "catalog/items.h"
#include <cstdint>

struct ItemStack {
    Catalog::ItemID id    {};
    uint32_t        count {};  // TODO: Check this is < itemCatalog[id].stackLimit

    const char *Name() const
    {
        const Catalog::Item &item = Catalog::g_items.FindById(id);
        return count == 1 ? item.namePlural : item.nameSingular;
    }
};
