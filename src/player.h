#pragma once
#include "body.h"
#include "combat.h"
#include <stdbool.h>

typedef enum PlayerAction {
    PlayerAction_None   = 0,
    PlayerAction_Attack = 1,
} PlayerAction;

typedef enum PlayerAttachPoint {
    PlayerAttachPoint_Gut
} PlayerAttachPoint;

typedef struct Player {
    const char *name;
    Body3D body;
    Combat combat;
    PlayerAction action;
} Player;

void player_init                    (Player *player, const char *name, struct Sprite *sprite);
Vector3 player_get_attach_point     (const Player *player, PlayerAttachPoint attachPoint);
bool player_move                    (Player *player, double now, double dt, Vector2 offset);
bool player_attack                  (Player *player, double now, double dt);
void player_update                  (Player *player, double now, double dt);
void player_draw                    (Player *player);