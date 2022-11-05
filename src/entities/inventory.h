#pragma once
#include "facet.h"
#include "../catalog/items.h"
#include <cstdint>

#define PLAYER_INV_ROWS 4
#define PLAYER_INV_COLS 10
#define PLAYER_INV_REG_COUNT (PLAYER_INV_ROWS * PLAYER_INV_COLS)

typedef uint8_t SlotId;
enum : SlotId {
    SlotId_Coin_Copper = PLAYER_INV_REG_COUNT,
    SlotId_Coin_Silver,
    SlotId_Coin_Gilded,
    SlotId_Hotbar_0,
    SlotId_Hotbar_1,
    SlotId_Hotbar_2,
    SlotId_Hotbar_3,
    SlotId_Hotbar_4,
    SlotId_Hotbar_5,
    SlotId_Hotbar_6,
    SlotId_Hotbar_7,
    SlotId_Hotbar_8,
    SlotId_Hotbar_9,
    SlotId_Cursor,
    SlotId_Count
};

typedef bool (*ItemFilterFn)(Item item);
struct Slot {
    ItemFilterFn filter {};
    ItemStack    stack  {};
};

struct Inventory : public Facet {
    SlotId   selectedSlot {};  // NOTE: for hotbar, needs rework
    // NOTE: If you want dynamic slots per inventory, do this instead:
    //Slot *slots;
    //size_t   slotCount
    // DO NOT use std::vector because C++ sucks, and Inventory is inside of
    // a union if you follow enough compositions upward.
    Slot     slots        [SlotId_Count]{};
    bool     dirty        {true};  // Used server-side to determine whether client needs a new inv snapshot
    bool     skipUpdate   {};      // HACK: Simulate inv action to see if it's valid client-side without actually performing it

    static int Compare                             (Slot &a, Slot &b, bool ignoreEmpty = false);

    ItemStack  GetSelectedStack                    (void) const;
    void       TexRect                             (const Texture &invItems, ItemType itemId, Vector2 &min, Vector2 &max);
    bool       PickUp                              (ItemStack &srcStack); // Returns true if something was picked up by player
    Slot &     CursorSlot                          ();
    Slot &     GetInvSlot                          (SlotId slotId);
    void       SwapSlots                           (Slot &a, Slot &b);
    bool       TransferSlot                        (Slot &src, Slot &dst, uint32_t transferLimit = 0, bool skipFull = false, bool *dstFull = 0);
    bool       SlotClick                           (SlotId slotId, bool doubleClicked);
    bool       SlotScroll                          (SlotId slotId, int scroll);
    ItemStack  SlotDrop                            (SlotId slotId, uint32_t count);
    void       Sort                                (bool ignoreEmpty = false);
    void       Combine                             (bool ignoreEmpty = false);
    void       SortAndCombine                      (bool ignoreEmpty = false);
    bool       ItemFilter_ItemClass_Weapon         (Item item);
    bool       ItemFilter_ItemType_Currency_Copper (Item item);
    bool       ItemFilter_ItemType_Currency_Silver (Item item);
    bool       ItemFilter_ItemType_Currency_Gilded (Item item);
};