#pragma once
#include "body.h"
#include "combat.h"
#include "player_inventory.h"
#include "spritesheet.h"
#include <stdbool.h>

typedef enum PlayerMoveState {
    PlayerMoveState_Idle      = 0,
    PlayerMoveState_Walking   = 1,
    PlayerMoveState_Running   = 2,
} PlayerMoveState;

typedef enum PlayerActionState {
    PlayerActionState_None      = 0,
    PlayerActionState_Attacking = 1,
} PlayerActionState;

typedef struct PlayerInput {
    PlayerInventorySlot selectedSlot;
    PlayerMoveState moveState;
    Direction direction;
    PlayerActionState actionState;
} PlayerInput;

typedef enum PlayerAttachPoint {
    PlayerAttachPoint_Gut
} PlayerAttachPoint;

typedef struct Player {
    const char *name;
    Body3D body;
    Combat combat;
    PlayerMoveState moveState;
    PlayerActionState actionState;
} Player;

void player_init                (Player *player, const char *name, struct Sprite *sprite);
Vector3 player_get_attach_point (const Player *player, PlayerAttachPoint attachPoint);
bool player_move                (Player *player, double now, double dt, Vector2 offset);
bool player_attack              (Player *player, double now, double dt);
void player_update              (Player *player, double now, double dt);
void player_draw                (Player *player);