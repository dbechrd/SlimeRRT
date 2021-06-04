#pragma once

enum ItemType {
    ItemType_Empty,
    ItemType_Currency,
    ItemType_Weapon,
    ItemType_Count
};

enum ItemID {
    ItemID_Empty,
    ItemID_Currency_Coin,
    ItemID_Weapon_Sword,
    ItemID_Count
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
