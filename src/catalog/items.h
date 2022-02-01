#pragma once
#include <array>

namespace Catalog {
    enum class ItemID {
        Empty,
        Currency_Copper,
        Currency_Silver,
        Currency_Gilded,
        Weapon_Sword,
        Count
    };

    enum class ItemType {
        Empty,
        Currency,
        Weapon,
        Count
    };

    struct Item {
        ItemID   id         {};
        ItemType type       {};
        uint32_t stackLimit {};
        float    value      {};
        float    damage     {};
    };

    struct Items {
        void Load(void);
        const Item &FindById(ItemID id) const;
        const char *Name(ItemID id, bool plural = false) const;

    private:
        std::array<Item, (size_t)ItemID::Count> byId {};
    } g_items;
}