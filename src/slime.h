#pragma once
#include "raylib.h"
#include "spritesheet.h"
#include "body.h"
#include <stdbool.h>

typedef enum SlimeFacing {
    SlimeFacing_South,
    SlimeFacing_North,
    SlimeFacing_East,
    SlimeFacing_West,
    SlimeFacing_Count
} SlimeFacing;

typedef enum SlimeAction {
    SlimeAction_None   = 0,
    SlimeAction_Attack = 1,
} SlimeAction;

typedef struct Slime {
    const char *name;
    Body3D body;
    float scale;
    SlimeFacing facing;
    SlimeAction action;
    double attackStartedAt;
    double attackDuration;
    float maxHitPoints;
    float hitPoints;
    Sprite *sprite;
    size_t spriteFrameIdx;
} Slime;

void slime_init                     (Slime *slime, const char *name, struct Sprite *sprite);
SpriteFrame *slime_get_frame        (const Slime *slime);
Rectangle slime_get_frame_rect      (const Slime *slime);
Rectangle slime_get_rect            (const Slime *slime);
Vector3 slime_get_center            (const Slime *slime);
Vector2 slime_get_ground_position   (const Slime *slime);
void slime_move                     (Slime *slime, Vector2 offset);
void slime_combine                  (Slime *slimeA, Slime *slimeB);
bool slime_attack                   (Slime *slime);
void slime_update                   (Slime *slime);
void slime_draw                     (Slime *slime);