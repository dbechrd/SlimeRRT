#pragma once
#include "body.h"
#include "combat.h"
#include "sprite.h"
#include <stdbool.h>

#define MAX_SLIMES 512

typedef enum SlimeAction {
    SlimeAction_None   = 0,
    SlimeAction_Attack = 1,
} SlimeAction;

typedef struct Slime {
    const char *name;
    Body3D body;
    Combat combat;
    Sprite sprite;
    SlimeAction action;
    double randJumpIdle;
} Slime;

void slime_init    (Slime *slime, const char *name, const struct SpriteDef *spriteDef);
bool slime_move    (Slime *slime, double now, double dt, Vector2 offset);
bool slime_combine (Slime *slimeA, Slime *slimeB);
bool slime_attack  (Slime *slime, double now, double dt);
void slime_update  (Slime *slime, double now, double dt);
float slime_depth  (const Slime *slime);
bool slime_cull    (const Slime* slime, Rectangle cullRect);
void slime_push    (const Slime *slime);
void slime_draw    (const Slime *slime);
