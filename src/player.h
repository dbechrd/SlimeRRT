#pragma once
#include "body.h"
#include "combat.h"
#include "sprite.h"
#include "player_inventory.h"

struct Player {
    enum MoveState {
        MoveState_Idle      = 0,
        MoveState_Walking   = 1,
        MoveState_Running   = 2,
    };

    enum ActionState {
        ActionState_None      = 0,
        ActionState_Attacking = 1,
    };

    enum AttachPoint {
        AttachPoint_Gut
    };

    struct Stats {
        unsigned int coinsCollected;
        float damageDealt;
        float kmWalked;
        unsigned int slimesSlain;
        unsigned int timesFistSwung;
        unsigned int timesSwordSwung;
    };

    const char *    m_name;
    ActionState     m_actionState;
    MoveState       m_moveState;
    Body3D          m_body;
    Combat          m_combat;
    Sprite          m_sprite;
    PlayerInventory m_inventory;
    Stats           m_stats;

    Player(const char *name, const SpriteDef &spriteDef);

    Vector3 GetAttachPoint(AttachPoint attachPoint) const;
    const Item& GetSelectedItem() const;

    bool Move(double now, double dt, Vector2 offset);
    bool Attack(double now, double dt);
    void Update(double now, double dt);
    float Depth() const;
    bool Cull(const Rectangle &cullRect) const;
    void Push() const;
    void Draw() const;

private:
    Player() = default;

    void UpdateDirection(Vector2 offset);
};