#include "inventory.h"

ItemStack Inventory::GetSelectedStack(void) const
{
    ItemStack stack = slots[(size_t)selectedSlot].stack;
    if (stack.uid || stack.count) {
        DLB_ASSERT(stack.uid);
        DLB_ASSERT(stack.count);
    }
    return stack;
}

void Inventory::TexRect(const Texture &invItems, ItemType itemId, Vector2 &min, Vector2 &max)
{
    const int texIdx = (int)itemId;
    const int texItemsWide = invItems.width / ITEM_W;
    //const int texItemsHigh = invItems.height / ITEM_H;
    const float u0 = (float)((texIdx % texItemsWide) * ITEM_W);
    const float v0 = (float)((texIdx / texItemsWide) * ITEM_H);
    min = Vector2{ (u0) / invItems.width, (v0) / invItems.height };
    max = Vector2{ (u0 + ITEM_W) / invItems.width, (v0 + ITEM_H) / invItems.height };
}

// Returns true if something was picked up by player
bool Inventory::PickUp(ItemStack &srcStack)
{
    if (!srcStack.count) {
        return false;
    }

    Slot srcSlot{};
    srcSlot.stack = srcStack;

    for (SlotId slotId = 0; slotId < SlotId_Count; slotId++) {
        Slot &dstSlot = GetInvSlot(slotId);
        TransferSlot(srcSlot, dstSlot);
        if (!srcStack.count) {
            break;
        }
    }

    bool pickup = srcSlot.stack.count != srcStack.count;
    srcStack.count = srcSlot.stack.count;
    return pickup;
}

Slot &Inventory::CursorSlot()
{
    Slot &cursor = slots[SlotId_Cursor];
    return cursor;
}

Slot &Inventory::GetInvSlot(SlotId slotId)
{
    DLB_ASSERT(slotId >= 0);
    DLB_ASSERT(slotId < SlotId_Count);
    Slot &slot = slots[slotId];
    return slot;
}

void Inventory::SwapSlots(Slot &a, Slot &b)
{
    // TODO: Check slot filter
    if (!skipUpdate) {
        ItemStack tmp = a.stack;
        a.stack = b.stack;
        b.stack = tmp;
        dirty = true;
    }
}

bool Inventory::TransferSlot(Slot &src, Slot &dst, uint32_t transferLimit, bool skipFull, bool *dstFull)
{
    // Nothing to transfer
    if (!src.stack.count) {
        return false;
    }

    // TODO: Check slot filter

    const Item &item = g_item_db.Find(src.stack.uid);

    // If dst slot has an item already, only transfer if items are an exact match (including all affixes)
    if (dst.stack.count && dst.stack.uid != item.uid) {
        return false;
    }

    uint32_t stackLimit = item.Proto().stackLimit;

    // Dst slot already full
    if (dst.stack.count == stackLimit) {
        return false;
    }

    // Don't transfer full stacks from src (e.g. when collecting items on double-click)
    if (skipFull && src.stack.count == stackLimit) {
        return false;
    }

    // Transfer as many items as possible from src to dst
    uint32_t dstFreeSpace = stackLimit - dst.stack.count;
    uint32_t maxCanTransfer = MIN(src.stack.count, dstFreeSpace);
    uint32_t transfer = transferLimit ? MIN(maxCanTransfer, transferLimit) : maxCanTransfer;
    if (!skipUpdate) {
        src.stack.count -= transfer;
        if (!src.stack.count) {
            src.stack.uid = 0;
        }
        dst.stack.count += transfer;
        if (dst.stack.count) {
            dst.stack.uid = item.uid;
        }
        dirty = true;
    }
    if (dstFull) *dstFull = (dst.stack.count == stackLimit);
    return transfer > 0;
}

bool Inventory::SlotClick(SlotId slotId, bool doubleClicked)
{
    Slot &cursor = CursorSlot();
    Slot &slot = GetInvSlot(slotId);

    bool success = false;
    if (doubleClicked) {
        if (!cursor.stack.count) {
            if (!slot.stack.count) {
                return false;
            }
            SwapSlots(cursor, slot);
        }

        // Pick up as many items as possible from all slots that match the cursor's current item
        bool dstFull = false;
        for (int otherSlotId = SlotId_Count - 1; otherSlotId >= 0 && !dstFull; otherSlotId--) {
            if (otherSlotId == SlotId_Cursor)
                continue;

            Slot &otherSlot = GetInvSlot(otherSlotId);
            success |= TransferSlot(otherSlot, cursor, 0, true, &dstFull);
        }
    } else if (slot.stack.uid == cursor.stack.uid) {
        // Place as many items as possible into clicked slot
        success = TransferSlot(cursor, slot);
    } else {
        // Pick up / set down all items to/from clicked slot
        SwapSlots(cursor, slot);
        success = true;
    }
    return success;
}

bool Inventory::SlotScroll(SlotId slotId, int scroll)
{
    const int transfer = scroll;
    if (!transfer) {
        return false;
    }

    Slot &cursor = CursorSlot();
    Slot &slot = GetInvSlot(slotId);
    bool success = false;
    if (transfer > 0) {
        success = TransferSlot(cursor, slot, (uint32_t)transfer, false);
    } else {
        success = TransferSlot(slot, cursor, (uint32_t)-transfer, false);
    }
    return success;
}

ItemStack Inventory::SlotDrop(SlotId slotId, uint32_t count)
{
    Slot dropSlot{};
    TransferSlot(GetInvSlot(slotId), dropSlot, count, false);
    return dropSlot.stack;
}

int Inventory::Compare(Slot &a, Slot &b, bool ignoreEmpty)
{
    if (a.stack.uid == b.stack.uid) {
        return 0;
    } else if (!a.stack.uid) {
        return ignoreEmpty ? 0 : 1;
    } else if (!b.stack.uid) {
        return ignoreEmpty ? 0 : -1;
    } else {
        return a.stack.uid < b.stack.uid ? -1 : 1;
    }
}

void Inventory::Sort(bool ignoreEmpty)
{
    for (int slotIdxA = 0; slotIdxA < PLAYER_INV_REG_COUNT; slotIdxA++) {
        Slot &slotA = GetInvSlot(slotIdxA);

        for (int slotIdxB = slotIdxA + 1; slotIdxB < PLAYER_INV_REG_COUNT; slotIdxB++) {
            Slot &slotB = GetInvSlot(slotIdxB);

            if (Compare(slotA, slotB, ignoreEmpty) > 0) {
                SwapSlots(slotA, slotB);
            }
        }
    }
}

void Inventory::Combine(bool ignoreEmpty)
{
    for (int slotIdxA = 0; slotIdxA < PLAYER_INV_REG_COUNT; slotIdxA++) {
        Slot &slotA = GetInvSlot(slotIdxA);
        if (ignoreEmpty && !slotA.stack.count) continue;

        for (int slotIdxB = slotIdxA + 1; slotIdxB < PLAYER_INV_REG_COUNT; slotIdxB++) {
            Slot &slotB = GetInvSlot(slotIdxB);
            if (ignoreEmpty && !slotB.stack.count) continue;

            TransferSlot(slotB, slotA);
        }
    }
}

void Inventory::SortAndCombine(bool ignoreEmpty)
{
    Sort(ignoreEmpty);
    Combine(ignoreEmpty);
}

bool Inventory::ItemFilter_ItemClass_Weapon(Item item)
{
    return item.Proto().itemClass == ItemClass_Weapon;
};

bool Inventory::ItemFilter_ItemType_Currency_Copper(Item item)
{
    return item.type == ItemType_Currency_Copper;
};

bool Inventory::ItemFilter_ItemType_Currency_Silver(Item item)
{
    return item.type == ItemType_Currency_Silver;
};

bool Inventory::ItemFilter_ItemType_Currency_Gilded(Item item)
{
    return item.type == ItemType_Currency_Gilded;
};