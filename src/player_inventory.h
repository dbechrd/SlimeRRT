#pragma once
#include "item.h"

enum class PlayerInventorySlot {
    Slot_0 = 0,
    Slot_1 = 1,
    Slot_2 = 2,
    Slot_3 = 3,
    Slot_4 = 4,
    Slot_5 = 5,
    Slot_6 = 6,
    Coins  = 7,
    Count
};

struct PlayerInventory {
    PlayerInventorySlot selectedSlot{};
    ItemStack slots[(int)PlayerInventorySlot::Count]{};
};