#include "item_catalog.h"
#include "item.h"
#include "dlb_types.h"
#include <cassert>

ItemCatalog ItemCatalog::instance{};

void ItemCatalog::Load()
{
    byId[(size_t)ItemID::Currency_Coin] = &currencies.emplace_back(ItemID::Currency_Coin, 1.0f);
    byId[(size_t)ItemID::Weapon_Sword] = &weapons.emplace_back(ItemID::Weapon_Sword, 5.0f);
}

const Item &ItemCatalog::FindById(ItemID id) const
{
    const Item *item = byId[(size_t)id];
    return item ? *item : empty;
}