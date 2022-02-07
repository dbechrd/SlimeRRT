#pragma once
#include "body.h"
#include "catalog/items.h"
#include "combat.h"
#include "draw_command.h"
#include "item_stack.h"
#include "ring_buffer.h"
#include "sprite.h"

struct Tilemap;

#define PLAYER_INV_ROWS 6
#define PLAYER_INV_COLS 10

enum class PlayerInventorySlot {
    None,
    Coin_Copper,
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
    Count
};

struct PlayerInventory {
    PlayerInventorySlot selectedSlot{};  // NOTE: for hotbar, needs rework
    ItemStack cursor{};
    Vector2 cursorOffset{};
    ItemStack slots[(int)PlayerInventorySlot::Count + (PLAYER_INV_ROWS * PLAYER_INV_COLS)]{};

    ItemStack &GetInvStack(int row, int col)
    {
        assert(row >= 0);
        assert(col >= 0);
        assert(row < PLAYER_INV_ROWS);
        assert(row < PLAYER_INV_COLS);
        const int slot = row * PLAYER_INV_COLS + col;
        ItemStack &stack = slots[(int)PlayerInventorySlot::Count + slot];
        return stack;
    }

    void SlotClick(int row, int col)
    {
        ItemStack &invStack = GetInvStack(row, col);

        if (cursor.count && cursor.id == invStack.id) {
            invStack.count += cursor.count;
            cursor = {};
        } else {
            ItemStack tmp = cursor;
            cursor = invStack;
            invStack = tmp;
        }
    }
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