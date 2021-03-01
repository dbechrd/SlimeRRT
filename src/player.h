#pragma once
#include "raylib.h"
#include "spritesheet.h"

typedef enum PlayerState {
    PlayerState_Idle      = 0,
    PlayerState_West      = 1,
    PlayerState_East      = 2,
    PlayerState_South     = 3,
    PlayerState_North     = 4,
    PlayerState_NorthEast = 5,
    PlayerState_NorthWest = 6,
    PlayerState_SouthWest = 7,
    PlayerState_SouthEast = 8
} PlayerState;

typedef enum PlayerAttachPoint {
    PlayerAttachPoint_Gut
} PlayerAttachPoint;

typedef struct Player {
    const char *name;
    PlayerState state;
    Vector2 position;
    double lastMoveTime;
    Sprite *sprite;
    size_t spriteFrameIdx;
} Player;

void player_init                (Player *player, const char *name, struct Sprite *sprite);
SpriteFrame *player_get_frame   (Player *player);
Rectangle player_get_frame_rect (Player *player);
Rectangle player_get_rect       (Player *player);
Vector2 player_get_center       (Player *player);
Vector2 player_get_attach_point (Player *player, PlayerAttachPoint attachPoint);
void player_move                (Player *player, Vector2 offset);
void player_update              (Player *player);
void player_draw                (Player *player);