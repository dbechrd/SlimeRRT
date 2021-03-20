#pragma once
#include "item.h"

typedef enum {
    PlayerInventorySlot_1     = 0,
    PlayerInventorySlot_2     = 1,
    PlayerInventorySlot_3     = 2,
    PlayerInventorySlot_Coins = 3,
    PlayerInventorySlot_Count
} PlayerInventorySlot;

typedef struct {
    ItemStack slots[PlayerInventorySlot_Count];
} PlayerInventory;