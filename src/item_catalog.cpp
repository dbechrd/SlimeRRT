#include "item_catalog.h"
#include "item.h"
#include "dlb_types.h"
#include <cassert>

ItemCatalog g_itemCatalog{};

ItemCatalog::ItemCatalog()
{
    byId = new std::array<Item *, (int)ItemID::Count>;
    byId->at((int)ItemID::Currency_Coin) = &currencies.emplace_back(ItemID::Currency_Coin, 1.0f);
    byId->at((int)ItemID::Weapon_Sword) = &weapons.emplace_back(ItemID::Weapon_Sword, 5.0f);
}

ItemCatalog::~ItemCatalog()
{
    delete byId;
}

const Item &ItemCatalog::FindById(ItemID id) const
{
    const Item *item = byId->at((int)id);
    return item ? *item : empty;
}