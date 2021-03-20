#include "item_catalog.h"
#include "item.h"
#include "dlb_types.h"
#include <assert.h>

static Item itemCatalog[ItemID_Count];

void item_catalog_init()
{
    // TODO: Load items from file
    itemCatalog[ItemID_Empty              ] = (Item){ ItemID_Empty              ,   1, ItemType_Empty    };
    itemCatalog[ItemID_Currency_Coin      ] = (Item){ ItemID_Currency_Coin      , 999, ItemType_Currency };
    itemCatalog[ItemID_Weapon_Sword       ] = (Item){ ItemID_Weapon_Sword       ,   1, ItemType_Weapon   };
    itemCatalog[ItemID_Weapon_Sword       ].data.weapon.damage = 5.0f;
}

const Item *item_catalog_find(ItemID id)
{
    // TODO: Return null if invalid id?
    assert(id < ARRAY_SIZE(itemCatalog));
    return &itemCatalog[id];
}
