#pragma once
#include <stdint.h>

enum class ItemID {
    Empty,
    Currency_Coin,
    Weapon_Sword,
    Count
};

struct ItemStack {
    ItemID   id         {};
    uint32_t stackCount {};  // TODO: Check this is < itemCatalog[id].stackLimit
};

enum class ItemType {
    Empty,
    Currency,
    Weapon,
    Count
};

struct Item {
    const ItemType type       {};
    const ItemID   id         {};
    const uint32_t stackLimit {};

    Item(ItemType type, ItemID id, uint32_t stackLimit) : type{type}, id{id}, stackLimit{stackLimit} {};
};

struct Item_Currency : Item {
    const float value{};

    Item_Currency(ItemID id, float value) : Item(ItemType::Currency, id, 99), value{value} {};
};

// TODO: Min damage, max damage, effects, etc.
struct Item_Weapon : Item {
    const float damage{};

    Item_Weapon(ItemID id, float damage) : Item(ItemType::Weapon, id, 1), damage{damage} {};
};