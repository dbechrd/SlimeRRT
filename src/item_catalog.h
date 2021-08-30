#pragma once
#include "item.h"
#include <array>
#include <vector>

struct ItemCatalog {
    static ItemCatalog instance;

    void ItemCatalog::Load();
    const Item &FindById(ItemID id) const;

private:
    ItemCatalog() = default;
    const Item empty{ItemType::Empty, ItemID::Empty, 0};

    std::array<Item *, (int)ItemID::Count> byId {};
    std::vector<Item_Currency> currencies {};
    std::vector<Item_Weapon>   weapons    {};
};