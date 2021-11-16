#pragma once
#include "body.h"
#include "combat.h"
#include "draw_command.h"
#include "sprite.h"
#include "player_inventory.h"

struct Player {
    enum class MoveState {
        Idle      = 0,
        Walking   = 1,
        Running   = 2,
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
    char            name[USERNAME_LENGTH_MAX]{};
    ActionState     actionState {};
    MoveState       moveState   {};
    Body3D          body        {};
    Combat          combat      {};
    Sprite          sprite      {};
    PlayerInventory inventory   {};
    Stats           stats       {};

    void Init();
    void SetName(const char *name, uint32_t nameLength);

    Vector3 GetAttachPoint(AttachPoint attachPoint) const;
    const Item& GetSelectedItem() const;

    bool Move(double now, double dt, Vector2 offset);
    bool Attack(double now, double dt);
    void Update(double now, double dt);
    void Push(DrawList &drawList) const;

private:
    void UpdateDirection(Vector2 offset);
};

float Player_Depth(const Drawable &drawable);
bool  Player_Cull (const Drawable &drawable, const Rectangle &cullRect);
void  Player_Draw (const Drawable &drawable);