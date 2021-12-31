#pragma once
#include "body.h"
#include "catalog/items.h"
#include "combat.h"
#include "draw_command.h"
#include "item_stack.h"
#include "ring_buffer.h"
#include "sprite.h"

struct Tilemap;

enum class PlayerInventorySlot {
    None = 0,
    Slot_1 = 1,
    Slot_2 = 2,
    Slot_3 = 3,
    Slot_4 = 4,
    Slot_5 = 5,
    Slot_6 = 6,
    Coins = 7,
    Count
};

struct PlayerInventory {
    PlayerInventorySlot selectedSlot{};
    ItemStack slots[(int)PlayerInventorySlot::Count]{};
};

struct Player : Drawable {
    enum class MoveState {
        Idle      = 0,
        Walking   = 1,
        Running   = 2,
    };

    enum class ActionState {
        None        = 0,
        AttackStart = 1,
        Attacking   = 2,
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
    ActionState     actionState {};
    MoveState       moveState   {};
    Vector2         moveBuffer  {};
    Body3D          body        {};
    Combat          combat      {};
    Sprite          sprite      {};
    PlayerInventory inventory   {};
    Stats           stats       {};

    void Init(const SpriteDef *spriteDef);
    void SetName(const char *name, uint32_t nameLength);
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