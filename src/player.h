#pragma once
#include "body.h"
#include "combat.h"
#include "sprite.h"
#include "player_inventory.h"

enum PlayerMoveState {
    PlayerMoveState_Idle      = 0,
    PlayerMoveState_Walking   = 1,
    PlayerMoveState_Running   = 2,
};

enum PlayerActionState {
    PlayerActionState_None      = 0,
    PlayerActionState_Attacking = 1,
};

enum PlayerAttachPoint {
    PlayerAttachPoint_Gut
};

struct PlayerStats {
    unsigned int coinsCollected;
    float damageDealt;
    float kmWalked;
    unsigned int slimesSlain;
    unsigned int timesFistSwung;
    unsigned int timesSwordSwung;
};

struct Player {
public:
    Player(const char *name, const SpriteDef &spriteDef);

public:
    Vector3 GetAttachPoint(PlayerAttachPoint attachPoint) const;
    const Item& GetSelectedItem() const;

private:
    Player() = default;

public:
    bool Move(double now, double dt, Vector2 offset);
    bool Attack(double now, double dt);
    void Update(double now, double dt);
    float Depth() const;
    bool Cull(const Rectangle &cullRect) const;
    void Push() const;
    void Draw() const;

private:
    void UpdateDirection(Vector2 offset);

public:
    const char       *name;
    PlayerActionState actionState;
    PlayerMoveState   moveState;
    Body3D            body;
    Combat            combat;
    Sprite            sprite;
    PlayerInventory   inventory;
    PlayerStats       stats;
};