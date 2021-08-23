#pragma once
#include "body.h"
#include "combat.h"
#include "drawable.h"
#include "sprite.h"
#include "player_inventory.h"

struct Player : public Drawable {
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

    const char *    name        {};
    ActionState     actionState {};
    MoveState       moveState   {};
    Body3D          body        {};
    Combat          combat      {};
    PlayerInventory inventory   {};
    Stats           stats       {};

    Player(const char *playerName, const SpriteDef &spriteDef);

    float Depth() const override;
    bool Cull(const Rectangle &cullRect) const override;
    void Draw() const override;

    Vector3 GetAttachPoint(AttachPoint attachPoint) const;
    const Item& GetSelectedItem() const;

    bool Move(double now, double dt, Vector2 offset);
    bool Attack(double now, double dt);
    void Update(double now, double dt);

private:
    Player() = default;

    void UpdateDirection(Vector2 offset);
};