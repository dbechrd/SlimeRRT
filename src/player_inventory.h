#pragma once
#include "item.h"

typedef enum {
    PlayerInventorySlot_None  = 0,
    PlayerInventorySlot_1     = 1,
    PlayerInventorySlot_2     = 2,
    PlayerInventorySlot_3     = 3,
    PlayerInventorySlot_4     = 4,
    PlayerInventorySlot_5     = 5,
    PlayerInventorySlot_6     = 6,
    PlayerInventorySlot_Coins = 7,
    PlayerInventorySlot_Count
} PlayerInventorySlot;

typedef struct {
    PlayerInventorySlot selectedSlot;
    ItemStack slots[PlayerInventorySlot_Count];
} PlayerInventory;