#include "catalog/items.h"

namespace Catalog {
    void Items::Load(void)
    {
        // TODO: Load from file (.csv?)
        byId[(size_t)ItemID::Empty        ] = { ItemID::Empty,         ItemType::Empty,     0,  0.0f, 0.0f };
        byId[(size_t)ItemID::Currency_Coin] = { ItemID::Currency_Coin, ItemType::Currency, 99,  1.0f, 0.0f };
        byId[(size_t)ItemID::Weapon_Sword ] = { ItemID::Weapon_Sword,  ItemType::Weapon,    1, 10.0f, 5.0f };
    }

    const Item &Items::FindById(ItemID id) const
    {
        return byId[(size_t)id];
    }
}