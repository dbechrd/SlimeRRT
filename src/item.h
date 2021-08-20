#pragma once

enum class ItemType {
    Empty,
    Currency,
    Weapon,
    Count
};

enum class ItemID {
    Empty,
    Currency_Coin,
    Weapon_Sword,
    Count
};

// TODO: Min damage, max damage, effects, etc.
struct Item_Weapon {
    float damage;
};

struct Item {
    ItemID id;
    unsigned int stackLimit;
    ItemType type;
    union {
        Item_Weapon weapon;
    } data;
};

struct ItemStack {
    ItemID id;
    unsigned int stackCount;
};
