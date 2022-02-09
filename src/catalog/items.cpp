#include "items.h"

namespace Catalog {
    void Items::Load(void)
    {
        // TODO: Load from file (.csv?)
        byId[(size_t)ItemID::Empty          ] = { ItemID::Empty,           ItemType::Empty,     0,     0.0f, 0.0f };
        byId[(size_t)ItemID::Currency_Copper] = { ItemID::Currency_Copper, ItemType::Currency,  9,     1.0f, 0.0f };
        byId[(size_t)ItemID::Currency_Silver] = { ItemID::Currency_Silver, ItemType::Currency,  9,   100.0f, 0.0f };
        byId[(size_t)ItemID::Currency_Gilded] = { ItemID::Currency_Gilded, ItemType::Currency,  9, 10000.0f, 0.0f };
        //byId[(size_t)ItemID::Currency_Copper] = { ItemID::Currency_Copper, ItemType::Currency, 99,     1.0f, 0.0f };
        //byId[(size_t)ItemID::Currency_Silver] = { ItemID::Currency_Silver, ItemType::Currency, 99,   100.0f, 0.0f };
        //byId[(size_t)ItemID::Currency_Gilded] = { ItemID::Currency_Gilded, ItemType::Currency, 99, 10000.0f, 0.0f };
        byId[(size_t)ItemID::Weapon_Sword   ] = { ItemID::Weapon_Sword,    ItemType::Weapon,    1,    10.0f, 5.0f };
    }

    const Item &Items::FindById(ItemID id) const
    {
        return byId[(size_t)id];
    }

    const char *Items::Name(ItemID id, bool plural) const
    {
#define PLURALIZE(one, many) {switch((int)plural) { case 0: return (one); case 1: return (many); }}
        switch (id) {
        case ItemID::Empty           : PLURALIZE("Empty", "Empty");
        case ItemID::Currency_Copper : PLURALIZE("Alloyed Piece", "Alloyed Pieces");  // brass
        case ItemID::Currency_Silver : PLURALIZE("Plated Piece", "Plated Pieces");  // sterling
        case ItemID::Currency_Gilded : PLURALIZE("Gilded Piece", "Gilded Pieces");  // shiny
        case ItemID::Weapon_Sword    : PLURALIZE("Sword", "Swords");
        default:
            return "<some item>";
        }
#undef NAME_PAIR
    }
}