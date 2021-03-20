#pragma once

typedef enum ItemType {
    ItemType_Empty,
    ItemType_Currency,
    ItemType_Weapon,
    ItemType_Count
} ItemType;

typedef enum ItemID {
    ItemID_Empty,
    ItemID_Currency_Coin,
    ItemID_Weapon_Sword,
    ItemID_Count
} ItemID;

// TODO: Min damage, max damage, effects, etc.
typedef struct Item_Weapon {
    float damage;
} Item_Weapon;

typedef struct Item {
    ItemID id;
    unsigned int stackLimit;
    ItemType type;
    union {
        Item_Weapon weapon;
    } data;
} Item;

typedef struct ItemStack {
    ItemID id;
    unsigned int stackCount;
} ItemStack;
