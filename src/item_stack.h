#pragma once
#include "catalog/items.h"
#include <cstdint>

struct ItemStack {
    Catalog::ItemID id    {};
    uint32_t        count {};  // TODO: Check this is < itemCatalog[id].stackLimit

    inline const Catalog::Item Item() const
    {
        const Catalog::Item item = Catalog::g_items.FindById(id);
        return item;
    }

    inline const Catalog::ItemType Type() {
        const Catalog::Item item = Item();
        return item.type;
    }

    inline const char *Name() const
    {
        const Catalog::Item item = Item();
        return count == 1 ? item.nameSingular : item.namePlural;
    }
};
