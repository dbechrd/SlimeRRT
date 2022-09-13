#pragma once
#include "body.h"
#include "catalog/items.h"
#include "combat.h"
#include "draw_command.h"
#include "enemy.h"
#include "catalog/items.h"
#include "ring_buffer.h"
#include "sprite.h"
#include "dlb_rand.h"

struct Tilemap;

#define PLAYER_INV_ROWS 4
#define PLAYER_INV_COLS 10
#define PLAYER_INV_REG_COUNT (PLAYER_INV_ROWS * PLAYER_INV_COLS)

typedef uint8_t PlayerInvSlot;

enum : PlayerInvSlot {
    PlayerInvSlot_Coin_Copper = PLAYER_INV_REG_COUNT,
    PlayerInvSlot_Coin_Silver,
    PlayerInvSlot_Coin_Gilded,
    PlayerInvSlot_Hotbar_0,
    PlayerInvSlot_Hotbar_1,
    PlayerInvSlot_Hotbar_2,
    PlayerInvSlot_Hotbar_3,
    PlayerInvSlot_Hotbar_4,
    PlayerInvSlot_Hotbar_5,
    PlayerInvSlot_Hotbar_6,
    PlayerInvSlot_Hotbar_7,
    PlayerInvSlot_Hotbar_8,
    PlayerInvSlot_Hotbar_9,
    PlayerInvSlot_Cursor,
    PlayerInvSlot_Count
};

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

struct InventorySlot {
    ItemFilterFn filter {};
    ItemStack stack  {};
};

struct PlayerInventory {
    PlayerInvSlot selectedSlot {};  // NOTE: for hotbar, needs rework
    InventorySlot slots        [PlayerInvSlot_Count]{};

    void TexRect(const Texture &invItems, ItemType itemId, Vector2 &min, Vector2 &max)
    {
        const int texIdx = (int)itemId;
        const int texItemsWide = invItems.width / ITEM_W;
        const int texItemsHigh = invItems.height / ITEM_H;
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

        InventorySlot srcSlot{};
        srcSlot.stack = srcStack;

        for (int slotIdx = 0; slotIdx < PlayerInvSlot_Count; slotIdx++) {
            InventorySlot &dstSlot = GetInvSlot(slotIdx);
            TransferSlot(srcSlot, dstSlot);
            if (!srcStack.count) {
                break;
            }
        }

        bool pickup = srcSlot.stack.count != srcStack.count;
        srcStack.count = srcSlot.stack.count;
        return pickup;
    }

    InventorySlot &CursorSlot()
    {
        InventorySlot &cursor = slots[PlayerInvSlot_Cursor];
        return cursor;
    }

    InventorySlot &GetInvSlot(int slotIdx)
    {
        assert(slotIdx >= 0);
        assert(slotIdx < PlayerInvSlot_Count);
        InventorySlot &slot = slots[slotIdx];
        return slot;
    }

    // TODO: Check slot filter
    void SwapSlots(InventorySlot &a, InventorySlot &b)
    {
        ItemStack tmp = a.stack;
        a.stack = b.stack;
        b.stack = tmp;
    }

    // Attempt to transfer some amount of items from one slot to another
    // TODO: Check slot filter
    bool TransferSlot(InventorySlot &src, InventorySlot &dst, bool skipFull = false, uint32_t transferLimit = UINT32_MAX)
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
        uint32_t transfer = MIN(maxCanTransfer, transferLimit);
        src.stack.count -= transfer;
        if (!src.stack.count) {
            src.stack.uid = 0;
        }
        dst.stack.count += transfer;
        if (dst.stack.count) {
            dst.stack.uid = item.uid;
        }
        return dst.stack.count == stackLimit;
    }

    void SlotClick(int slotIdx, bool doubleClicked)
    {
        InventorySlot &cursor = CursorSlot();
        InventorySlot &slot = GetInvSlot(slotIdx);

        if (doubleClicked) {
            if (!cursor.stack.count) {
                SwapSlots(cursor, slot);
            }

            // Pick up as many items as possible from all slots that match the cursor's current item
            bool done = false;
            for (int slotIdx = PlayerInvSlot_Count - 1; slotIdx >= 0 && !done; slotIdx--) {
                if (slotIdx == PlayerInvSlot_Cursor)
                    continue;

                InventorySlot &slot = GetInvSlot(slotIdx);
                done = TransferSlot(slot, cursor, true);
            }
        } else if (slot.stack.uid == cursor.stack.uid) {
            // Place as many items as possible into clicked slot
            TransferSlot(cursor, slot);
        } else {
            // Pick up / set down all items to/from clicked slot
            SwapSlots(cursor, slot);
        }
    }

    void SlotScroll(int slotIdx, int scroll)
    {
        const int transfer = scroll;
        if (!transfer) {
            return;
        }

        InventorySlot &cursor = CursorSlot();
        InventorySlot &slot = GetInvSlot(slotIdx);
        if (transfer > 0) {
            TransferSlot(cursor, slot, false, (uint32_t)transfer);
        } else {
            TransferSlot(slot, cursor, false, (uint32_t)-transfer);
        }
    }

    ItemStack SlotDrop(int slotIdx, uint32_t count)
    {
        InventorySlot dropSlot{};
        TransferSlot(GetInvSlot(slotIdx), dropSlot, false, count);
        return dropSlot.stack;
    }

    static int Compare(InventorySlot &a, InventorySlot &b, bool ignoreEmpty = false)
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
            InventorySlot &slotA = GetInvSlot(slotIdxA);

            for (int slotIdxB = slotIdxA + 1; slotIdxB < PLAYER_INV_REG_COUNT; slotIdxB++) {
                InventorySlot &slotB = GetInvSlot(slotIdxB);

                if (Compare(slotA, slotB, ignoreEmpty) > 0) {
                    SwapSlots(slotA, slotB);
                }
            }
        }
    }

    void Combine(bool ignoreEmpty = false)
    {
        for (int slotIdxA = 0; slotIdxA < PLAYER_INV_REG_COUNT; slotIdxA++) {
            InventorySlot &slotA = GetInvSlot(slotIdxA);
            if (ignoreEmpty && !slotA.stack.count) continue;

            for (int slotIdxB = slotIdxA + 1; slotIdxB < PLAYER_INV_REG_COUNT; slotIdxB++) {
                InventorySlot &slotB = GetInvSlot(slotIdxB);
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
        uint32_t enemiesSlain    [Enemy::Type_Count]{};
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
    void      Update           (InputSample &input, const Tilemap &map);

    float Depth (void) const;
    bool  Cull  (const Rectangle& cullRect) const;
    void  Draw  (World &world);

private:
    void UpdateDirection (Vector2 offset);
    bool Move            (Vector2 offset);
    bool Attack          (void);
    void DrawSwimOverlay (const World &world) const;
};