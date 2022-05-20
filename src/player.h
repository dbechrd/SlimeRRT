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

#define PLAYER_INV_ROWS 6
#define PLAYER_INV_COLS 10

enum class PlayerInventorySlot : uint8_t {
    Coin_Copper = (PLAYER_INV_ROWS * PLAYER_INV_COLS),
    Coin_Silver,
    Coin_Gilded,
    Hotbar_1,
    Hotbar_2,
    Hotbar_3,
    Hotbar_4,
    Hotbar_5,
    Hotbar_6,
    Hotbar_7,
    Hotbar_8,
    Cursor,
    Count
};

struct PlayerInventory {
    PlayerInventorySlot selectedSlot{};  // NOTE: for hotbar, needs rework
    ItemStack slots[(int)PlayerInventorySlot::Count]{};

    void TexRect(const Texture &invItems, Catalog::ItemID itemId, Vector2 &min, Vector2 &max)
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
        for (int slot = 0; slot < (int)PlayerInventorySlot::Count; slot++) {
            ItemStack &invStack = GetInvStack(slot);
            TransferStack(srcStack, invStack);
            if (!srcStack.count) {
                break;
            }
        }
        return srcStack.count < origCount;
    }

    ItemStack &CursorStack()
    {
        ItemStack &cursor = slots[(int)PlayerInventorySlot::Cursor];
        return cursor;
    }

    ItemStack &GetInvStack(int slot)
    {
        assert(slot >= 0);
        assert(slot < (int)PlayerInventorySlot::Count);
        ItemStack &stack = slots[slot];
        return stack;
    }

    void SwapStack(ItemStack &a, ItemStack &b)
    {
        ItemStack tmp = a;
        a = b;
        b = tmp;
    }

    // Transfer as many items as possible from cursor to inv stack
    bool TransferStack(ItemStack &src, ItemStack &dst, bool skipFull = false, uint32_t transferLimit = UINT32_MAX)
    {
        if (!dst.count || src.id == dst.id) {
            const Catalog::Item &item = Catalog::g_items.FindById(src.id);

            // Don't transfer full stacks (e.g. when collecting items on double-click)
            if (skipFull && src.count == item.stackLimit) {
                return false;
            }

            assert(dst.count <= item.stackLimit);
            uint32_t transferCount = MIN(MIN(src.count, item.stackLimit - dst.count), transferLimit);
            dst.count += transferCount;
            if (dst.count) {
                dst.id = src.id;
            }
            src.count -= transferCount;
            if (!src.count) {
                src.id = Catalog::ItemID::Empty;
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
            if (!invStack.count || (invStack.id == cursor.id)) {
                bool done = false;
                for (int slot = (int)PlayerInventorySlot::Count - 1; slot >= 0 && !done; slot--) {
                    ItemStack &srcStack = GetInvStack(slot);
                    done = TransferStack(srcStack, cursor, true);
                }
            }
        } else {
            if (cursor.count && cursor.id == invStack.id) {
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
        if (a.id == b.id) {
            return 0;
        } else if (a.id == Catalog::ItemID::Empty) {
            return ignoreEmpty ? 0 : 1;
        } else if (b.id == Catalog::ItemID::Empty) {
            return ignoreEmpty ? 0 : -1;
        //} else if (onlySameType && a.Type() != b.Type()) {
        //    return 0;
        } else {
            return (int)a.id < (int)b.id ? -1 : 1;
        }
    }

    void Sort(bool ignoreEmpty = false)
    {
        for (int slotA = 0; slotA < (int)PlayerInventorySlot::Count; slotA++) {
            ItemStack &invStackA = GetInvStack(slotA);

            for (int slotB = slotA + 1; slotB < (int)PlayerInventorySlot::Count; slotB++) {
                ItemStack &invStackB = GetInvStack(slotB);

                if (Compare(invStackA, invStackB, ignoreEmpty) > 0) {
                    SwapStack(invStackB, invStackA);
                }
            }
        }
    }

    void Combine(bool ignoreEmpty = false)
    {
        for (int slotA = 0; slotA < (int)PlayerInventorySlot::Count; slotA++) {
            ItemStack &invStackA = GetInvStack(slotA);
            const Catalog::ItemType typeA = invStackA.Type();
            if (ignoreEmpty && invStackA.id == Catalog::ItemID::Empty) continue;

            for (int slotB = slotA + 1; slotB < (int)PlayerInventorySlot::Count; slotB++) {
                ItemStack &invStackB = GetInvStack(slotB);
                const Catalog::ItemType typeB = invStackB.Type();
                if (ignoreEmpty && invStackB.id == Catalog::ItemID::Empty) continue;
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

#if _DEBUG
    void Randomize()
    {
        for (int row = 0; row < PLAYER_INV_ROWS; row++) {
            for (int col = 0; col < PLAYER_INV_COLS; col++) {
                int slot = row * PLAYER_INV_COLS + col;
                ItemStack &stack = GetInvStack(slot);
                stack.id = (Catalog::ItemID)dlb_rand32i_range(
                    0, 6
                    //(int)Catalog::ItemID::Empty,
                    //(int)Catalog::ItemID::Count - 1
                );
                const Catalog::ItemType type = stack.Type();
                if (type == Catalog::ItemType::System) {
                    stack.id = Catalog::ItemID::Empty;
                }
                stack.count = (int)stack.id != 0;
            }
        }
        //ItemStack &stack = GetInvStack(0, 0);
        //stack.count = 1;
        //stack.id = Catalog::ItemID::Weapon_Long_Sword;
    }
#endif
};

struct Player : Drawable {
    enum class MoveState {
        Idle    = 0,
        Walking = 1,
        Running = 2,
    };

    enum class ActionState {
        None      = 0,
        Attacking = 1,
    };

    enum class AttachPoint {
        Gut
    };

    struct Stats {
        uint32_t coinsCollected  {};
        float    damageDealt     {};
        float    kmWalked        {};
        uint32_t slimesSlain     {};
        uint32_t timesFistSwung  {};
        uint32_t timesSwordSwung {};
    };

    uint32_t        id          {};
    uint32_t        nameLength  {};
    char            name        [USERNAME_LENGTH_MAX]{};
    MoveState       moveState   {};
    ActionState     actionState {};
    Vector2         moveBuffer  {};
    Body3D          body        {};
    Combat          combat      {};
    Sprite          sprite      {};
    PlayerInventory inventory   {};
    Stats           stats       {};

    void Init(const SpriteDef *spriteDef);
    void SetName(const char *name, uint32_t nameLength);
    Vector3 WorldCenter(void) const;
    Vector3 WorldTopCenter(void) const;
    Vector3 GetAttachPoint(AttachPoint attachPoint) const;
    const ItemStack& GetSelectedItem() const;
    void Update (double dt, InputSample &input, const Tilemap &map);

    float Depth(void) const;
    bool  Cull(const Rectangle& cullRect) const;
    void  Draw(void) const;

private:
    void UpdateDirection (Vector2 offset);
    bool Move            (Vector2 offset);
    bool Attack          (void);
};