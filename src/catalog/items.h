#pragma once
#include <array>

namespace Catalog {
    enum class ItemID {
        Empty,
        Currency_Coin,
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
        int      stackLimit {};
        int      value      {};
        int      damage     {};
    };

    struct Items {
        void Load(void);
        const Item &FindById(ItemID id) const;

    private:
        std::array<Item, (size_t)ItemID::Count> byId {};
    } g_items;
}