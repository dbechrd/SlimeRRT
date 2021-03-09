#pragma once
#include "raylib.h"
#include "spritesheet.h"
#include "body.h"
#include <stdbool.h>

typedef enum PlayerFacing {
    PlayerFacing_Idle,
    PlayerFacing_West,
    PlayerFacing_East,
    PlayerFacing_South,
    PlayerFacing_North,
    PlayerFacing_NorthEast,
    PlayerFacing_NorthWest,
    PlayerFacing_SouthWest,
    PlayerFacing_SouthEast,
    PlayerFacing_Count
} PlayerFacing;

typedef enum PlayerWeapon {
    PlayerWeapon_None  = 0,
    PlayerWeapon_Sword = 1,
} PlayerWeapon;

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
    PlayerFacing facing;
    PlayerWeapon equippedWeapon;
    PlayerAction action;
    double lastMoveTime;
    double attackStartedAt;
    double attackDuration;
    float maxHitPoints;
    float hitPoints;
    Sprite *sprite;
    size_t spriteFrameIdx;
} Player;

void player_init                    (Player *player, const char *name, struct Sprite *sprite);
SpriteFrame *player_get_frame       (const Player *player);
Rectangle player_get_frame_rect     (const Player *player);
Rectangle player_get_rect           (const Player *player);
Vector3 player_get_center           (const Player *player);
Vector2 player_get_ground_position  (const Player *player);
Vector3 player_get_attach_point     (const Player *player, PlayerAttachPoint attachPoint);
bool player_move                    (Player *player, Vector2 offset);
bool player_attack                  (Player *player);
void player_update                  (Player *player);
void player_draw                    (Player *player);