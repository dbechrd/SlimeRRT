#pragma once
#include "body.h"
#include "catalog/items.h"
#include "combat.h"
#include "draw_command.h"
#include "item_stack.h"
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

typedef bool (*ItemFilterFn)(Catalog::Item item);

bool ItemFilter_ItemClass_Weapon(Catalog::Item item)
{
    return item.itemClass == ItemClass_Weapon;
};

bool ItemFilter_ItemType_Currency_Copper(Catalog::Item item)
{
    return item.itemType == ItemType_Currency_Copper;
};

bool ItemFilter_ItemType_Currency_Silver(Catalog::Item item)
{
    return item.itemType == ItemType_Currency_Silver;
};

bool ItemFilter_ItemType_Currency_Gilded(Catalog::Item item)
{
    return item.itemType == ItemType_Currency_Gilded;
};

struct InventorySlot {
    ItemFilterFn filter {};
    ItemStack    stack  {};
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
        uint32_t origCount = srcStack.count;
        for (int slot = 0; slot < PlayerInvSlot_Count; slot++) {
            ItemStack &invStack = GetInvStack(slot);
            TransferStack(srcStack, invStack);
            if (!srcStack.count) {
                break;
            }
        }
        return srcStack.count < origCount;
    }

    // TODO: CursorSlot
    ItemStack &CursorStack()
    {
        ItemStack &cursor = slots[PlayerInvSlot_Cursor].stack;
        return cursor;
    }

    // TODO: GetInvSlot
    ItemStack &GetInvStack(int slot)
    {
        assert(slot >= 0);
        assert(slot < PlayerInvSlot_Count);
        ItemStack &stack = slots[slot].stack;
        return stack;
    }

    // TODO: SwapSlot() and check filter
    void SwapStack(ItemStack &a, ItemStack &b)
    {
        ItemStack tmp = a;
        a = b;
        b = tmp;
    }

    // TODO: TransferSlot() and check filter
    // Transfer as many items as possible from one slot to another
    bool TransferStack(ItemStack &src, ItemStack &dst, bool skipFull = false, uint32_t transferLimit = UINT32_MAX)
    {
        if (!dst.count || src.itemType == dst.itemType) {
            const Catalog::Item &item = Catalog::g_items.Find(src.itemType);

            // Don't transfer full stacks (e.g. when collecting items on double-click)
            if (skipFull && src.count == item.stackLimit) {
                return false;
            }

            assert(dst.count <= item.stackLimit);
            uint32_t transferCount = MIN(MIN(src.count, item.stackLimit - dst.count), transferLimit);
            dst.count += transferCount;
            if (dst.count) {
                dst.itemType = src.itemType;
            }
            src.count -= transferCount;
            if (!src.count) {
                src.itemType = ITEMTYPE_EMPTY;
            }
            return dst.count == item.stackLimit;
        }
        return false;
    }

    void SlotClick(int slot, bool doubleClicked)
    {
        ItemStack &cursor = CursorStack();
        ItemStack &invStack = GetInvStack(slot);

        if (doubleClicked) {
            if (!cursor.count) {
                SwapStack(cursor, invStack);
            }
            if (!invStack.count || (invStack.itemType == cursor.itemType)) {
                bool done = false;
                for (int slot = PlayerInvSlot_Count - 1; slot >= 0 && !done; slot--) {
                    ItemStack &srcStack = GetInvStack(slot);
                    done = TransferStack(srcStack, cursor, true);
                }
            }
        } else {
            if (cursor.count && cursor.itemType == invStack.itemType) {
                TransferStack(cursor, invStack);
            } else {
                SwapStack(cursor, invStack);
            }
        }
    }

    void SlotScroll(int slot, int scroll)
    {
        const int transferAmount = scroll;
        if (transferAmount) {
            ItemStack &invStack = GetInvStack(slot);
            ItemStack &cursor = CursorStack();

            if (transferAmount > 0) {
                TransferStack(cursor, invStack, false, (uint32_t)transferAmount);
            } else {
                TransferStack(invStack, cursor, false, (uint32_t)-transferAmount);
            }
        }
    }

    ItemStack SlotDrop(int slot, uint32_t count)
    {
        ItemStack dropStack{};
        TransferStack(GetInvStack(slot), dropStack, false, count);
        return dropStack;
    }

    static int Compare(ItemStack &a, ItemStack &b, bool ignoreEmpty = false)
    {
        if (a.itemType == b.itemType) {
            return 0;
        } else if (a.itemType == ITEMTYPE_EMPTY) {
            return ignoreEmpty ? 0 : 1;
        } else if (b.itemType == ITEMTYPE_EMPTY) {
            return ignoreEmpty ? 0 : -1;
        //} else if (onlySameType && a.Type() != b.Type()) {
        //    return 0;
        } else {
            return (int)a.itemType < (int)b.itemType ? -1 : 1;
        }
    }

    void Sort(bool ignoreEmpty = false)
    {
        for (int slotA = 0; slotA < PLAYER_INV_REG_COUNT; slotA++) {
            ItemStack &invStackA = GetInvStack(slotA);

            for (int slotB = slotA + 1; slotB < PLAYER_INV_REG_COUNT; slotB++) {
                ItemStack &invStackB = GetInvStack(slotB);

                if (Compare(invStackA, invStackB, ignoreEmpty) > 0) {
                    SwapStack(invStackB, invStackA);
                }
            }
        }
    }

    void Combine(bool ignoreEmpty = false)
    {
        for (int slotA = 0; slotA < PLAYER_INV_REG_COUNT; slotA++) {
            ItemStack &invStackA = GetInvStack(slotA);
            const ItemClass typeA = invStackA.Type();
            if (ignoreEmpty && invStackA.itemType == ITEMTYPE_EMPTY) continue;

            for (int slotB = slotA + 1; slotB < PLAYER_INV_REG_COUNT; slotB++) {
                ItemStack &invStackB = GetInvStack(slotB);
                const ItemClass typeB = invStackB.Type();
                if (ignoreEmpty && invStackB.itemType == ITEMTYPE_EMPTY) continue;
                //if (onlySameType && typeA != typeB) continue;

                TransferStack(invStackB, invStackA);
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
        uint32_t slimesSlain     {};
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

    void              Init             (void);
    Vector3           WorldCenter      (void) const;
    Vector3           WorldTopCenter3D (void) const;
    Vector2           WorldTopCenter2D (void) const;
    Vector3           GetAttachPoint   (AttachPoint attachPoint) const;
    const ItemStack&  GetSelectedItem  (void) const;
    void              Update           (InputSample &input, const Tilemap &map);

    float Depth (void) const;
    bool  Cull  (const Rectangle& cullRect) const;
    void  Draw  (World &world);

private:
    void UpdateDirection (Vector2 offset);
    bool Move            (Vector2 offset);
    bool Attack          (void);
    void DrawSwimOverlay (const World &world) const;
};