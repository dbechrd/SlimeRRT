#include "items.h"

namespace Catalog {
    void Items::Load(void)
    {
        // TODO: Load from file (.csv?)

        byId[(size_t)ItemID::Empty            ] = { ItemID::Empty,             ItemType::Empty,      0,     0.0f, 0.0f, "Empty",         "Empty"          };
        byId[(size_t)ItemID::Currency_Copper  ] = { ItemID::Currency_Copper,   ItemType::Currency,  99,     1.0f, 0.0f, "Alloyed Piece", "Alloyed Pieces" };
        byId[(size_t)ItemID::Currency_Silver  ] = { ItemID::Currency_Silver,   ItemType::Currency,  99,   100.0f, 0.0f, "Plated Piece",  "Plated Pieces"  };
        byId[(size_t)ItemID::Currency_Gilded  ] = { ItemID::Currency_Gilded,   ItemType::Currency,  99, 10000.0f, 0.0f, "Gilded Piece",  "Gilded Pieces"  };
        byId[(size_t)ItemID::Weapon_Long_Sword] = { ItemID::Weapon_Long_Sword, ItemType::Weapon,     1,    10.0f, 5.0f, "Long Sword",    "Long Swords"  };

        // TODO(debug): Delete this after testing item stacking schtuff!
        byId[(size_t)ItemID::Currency_Copper].stackLimit = 9;
        byId[(size_t)ItemID::Currency_Silver].stackLimit = 9;
        byId[(size_t)ItemID::Currency_Gilded].stackLimit = 9;
    }

    const Item &Items::FindById(ItemID id) const
    {
        return byId[(size_t)id];
    }
}