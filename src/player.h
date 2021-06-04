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
    const char *name;
    PlayerActionState actionState;
    PlayerMoveState moveState;
    Body3D body;
    Combat combat;
    Sprite sprite;
    PlayerInventory inventory;
    PlayerStats stats;
};

void player_init                (Player *player, const char *name, const struct SpriteDef *spriteDef);
Vector3 player_get_attach_point (const Player *player, PlayerAttachPoint attachPoint);
const Item *player_selected_item(const Player *player);
bool player_move                (Player *player, double now, double dt, Vector2 offset);
bool player_attack              (Player *player, double now, double dt);
void player_update              (Player *player, double now, double dt);
float player_depth              (const Player *player);
bool player_cull                (const Player* player, Rectangle cullRect);
void player_push                (const Player *player);
void player_draw                (const Player *player);