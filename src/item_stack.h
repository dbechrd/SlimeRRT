#pragma once
#include "catalog/items.h"
#include <cstdint>

struct ItemStack {
    ItemType itemType {};
    uint32_t count    {};  // TODO: Check this is < itemCatalog[type].stackLimit

    inline const Catalog::Item &Item() const
    {
        const Catalog::Item &item = Catalog::g_items.Find(itemType);
        return item;
    }

    inline const ItemClass Type() {
        const Catalog::Item item = Item();
        return item.itemClass;
    }

    inline const char *Name() const
    {
        const Catalog::Item &item = Item();
        return count == 1 ? item.nameSingular : item.namePlural;
    }
};
