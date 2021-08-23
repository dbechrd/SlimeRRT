#pragma once
#include "item.h"
#include <array>
#include <vector>

struct ItemCatalog {
    ItemCatalog();
    ~ItemCatalog();
    const Item &FindById(ItemID id) const;

private:
    const Item empty{ItemType::Empty, ItemID::Empty, 0};

    std::array<Item *, (int)ItemID::Count> *byId {};
    std::vector<Item_Currency> currencies        {};
    std::vector<Item_Weapon> weapons             {};
};

extern ItemCatalog g_itemCatalog;