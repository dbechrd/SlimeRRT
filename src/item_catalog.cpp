#include "item_catalog.h"
#include "item.h"
#include "dlb_types.h"
#include <cassert>

static Item itemCatalog[(int)ItemID::Count];

void item_catalog_init()
{
    // TODO: Load items from file
    itemCatalog[(int)ItemID::Empty        ] = { ItemID::Empty        ,   1, ItemType::Empty    };
    itemCatalog[(int)ItemID::Currency_Coin] = { ItemID::Currency_Coin, 999, ItemType::Currency };
    itemCatalog[(int)ItemID::Weapon_Sword ] = { ItemID::Weapon_Sword ,   1, ItemType::Weapon   };
    itemCatalog[(int)ItemID::Weapon_Sword ].data.weapon.damage = 5.0f;
}

const Item& item_catalog_find(ItemID id)
{
    // TODO: Return null if invalid id?
    assert((int)id < ARRAY_SIZE(itemCatalog));
    return itemCatalog[(int)id];
}
