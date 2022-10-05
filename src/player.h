#pragma once
#include "body.h"
#include "catalog/items.h"
#include "combat.h"
#include "draw_command.h"
#include "entities/entities.h"
#include "catalog/items.h"
#include "catalog/sounds.h"
#include "ring_buffer.h"
#include "sprite.h"
#include "dlb_rand.h"

struct Tilemap;

#define PLAYER_INV_ROWS 4
#define PLAYER_INV_COLS 10
#define PLAYER_INV_REG_COUNT (PLAYER_INV_ROWS * PLAYER_INV_COLS)

typedef bool (*ItemFilterFn)(Item item);

bool ItemFilter_ItemClass_Weapon(Item item)
{
    return item.Proto().itemClass == ItemClass_Weapon;
};

bool ItemFilter_ItemType_Currency_Copper(Item item)
{
    return item.type == ItemType_Currency_Copper;
};

bool ItemFilter_ItemType_Currency_Silver(Item item)
{
    return item.type == ItemType_Currency_Silver;
};

bool ItemFilter_ItemType_Currency_Gilded(Item item)
{
    return item.type == ItemType_Currency_Gilded;
};

struct PlayerInventory {
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

    struct Slot {
        ItemFilterFn filter{};
        ItemStack stack{};
    };

    SlotId selectedSlot {};  // NOTE: for hotbar, needs rework
    Slot   slots        [SlotId_Count]{};
    bool   dirty        {true};  // Used server-side to determine whether client needs a new inv snapshot
    bool   skipUpdate   {};  // HACK: Simulate inv action to see if it's valid client-side without actually performing it

    void TexRect(const Texture &invItems, ItemType itemId, Vector2 &min, Vector2 &max)
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
    bool PickUp(ItemStack &srcStack)
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

    Slot &CursorSlot()
    {
        Slot &cursor = slots[SlotId_Cursor];
        return cursor;
    }

    Slot &GetInvSlot(SlotId slotId)
    {
        DLB_ASSERT(slotId >= 0);
        DLB_ASSERT(slotId < SlotId_Count);
        Slot &slot = slots[slotId];
        return slot;
    }

    // TODO: Check slot filter
    void SwapSlots(Slot &a, Slot &b)
    {
        if (!skipUpdate) {
            ItemStack tmp = a.stack;
            a.stack = b.stack;
            b.stack = tmp;
            dirty = true;
        }
    }

    // Attempt to transfer some amount of items from one slot to another
    // TODO: Check slot filter
    bool TransferSlot(Slot &src, Slot &dst, uint32_t transferLimit = 0, bool skipFull = false, bool *dstFull = 0)
    {
        // Nothing to transfer
        if (!src.stack.count) {
            return false;
        }

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

    bool SlotClick(SlotId slotId, bool doubleClicked)
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
            for (SlotId otherSlotId = SlotId_Count - 1; !dstFull; otherSlotId--) {
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

    bool SlotScroll(SlotId slotId, int scroll)
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

    ItemStack SlotDrop(SlotId slotId, uint32_t count)
    {
        Slot dropSlot{};
        TransferSlot(GetInvSlot(slotId), dropSlot, count, false);
        return dropSlot.stack;
    }

    static int Compare(Slot &a, Slot &b, bool ignoreEmpty = false)
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

    void Sort(bool ignoreEmpty = false)
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

    void Combine(bool ignoreEmpty = false)
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

    void SortAndCombine(bool ignoreEmpty = false)
    {
        Sort(ignoreEmpty);
        Combine(ignoreEmpty);
    }
};

struct PlayerInfo {
    uint32_t id         {};
    uint32_t nameLength {};
    char     name       [USERNAME_LENGTH_MAX]{};
    uint16_t ping       {};

    void SetName(const char *playerName, uint32_t playerNameLength)
    {
        nameLength = MIN(playerNameLength, USERNAME_LENGTH_MAX);
        memcpy(name, playerName, nameLength);
    }
};

struct Player : Drawable {
    enum class MoveState {
        Idle    = 0,
        Walking = 1,
        Running = 2,
    };

    enum class ActionState {
        None          = 0,
        AttackBegin   = 1,
        AttackSustain = 2,
        AttackRecover = 3,
    };

    enum class AttachPoint {
        Gut
    };

    struct Stats {
        //uint32_t coinsCollected  {};  // TODO: Money earned via selling? Or something else cool
        float    damageDealt     {};
        float    kmWalked        {};
        uint32_t npcsSlain       [NPC::Type_Count]{};
        uint32_t playersSlain    {};
        uint32_t timesFistSwung  {};
        uint32_t timesSwordSwung {};
    };

    uint32_t        id          {};
    MoveState       moveState   {};
    ActionState     actionState {};
    Vector2         moveBuffer  {};
    Body3D          body        {};
    Combat          combat      {};
    Sprite          sprite      {};
    uint32_t        xp          {};
    PlayerInventory inventory   {};
    Stats           stats       {};

    void      Init             (void);
    Vector3   WorldCenter      (void) const;
    Vector3   WorldTopCenter3D (void) const;
    Vector2   WorldTopCenter2D (void) const;
    Vector3   GetAttachPoint   (AttachPoint attachPoint) const;
    ItemStack GetSelectedStack (void) const;
    void      Update           (InputSample &input, Tilemap &map);

    float Depth (void) const override;
    bool  Cull  (const Rectangle& cullRect) const override;
    void  Draw  (World &world) override;

private:
    const char *LOG_SRC = "Player";

    void UpdateDirection (Vector2 offset);
    bool Move            (Vector2 offset);
    bool Attack          (void);
    void DrawSwimOverlay (const World &world) const;
};
