#include "item.h"
#include "helpers.h"
#include <assert.h>

static Item itemCatalog[ItemID_Count];

void item_catalog_init()
{
    itemCatalog[ItemID_Empty              ] = (Item){ ItemID_Empty              ,   1, ItemType_Empty    };
    itemCatalog[ItemID_Currency_Coin      ] = (Item){ ItemID_Currency_Coin      , 999, ItemType_Currency };
    itemCatalog[ItemID_Weapon_Melee_Player] = (Item){ ItemID_Weapon_Melee_Player,   1, ItemType_Weapon   };
    itemCatalog[ItemID_Weapon_Melee_Slime ] = (Item){ ItemID_Weapon_Melee_Slime ,   1, ItemType_Weapon   };
    itemCatalog[ItemID_Weapon_Sword       ] = (Item){ ItemID_Weapon_Sword       ,   1, ItemType_Weapon   };
}

const Item *item_by_id(ItemID id)
{
    // TODO: Return null if invalid id?
    assert(id < ARRAY_SIZE(itemCatalog));
    return &itemCatalog[id];
}
