#pragma once
#include "body.h"
#include "combat.h"
#include "spritesheet.h"
#include <stdbool.h>

typedef enum SlimeAction {
    SlimeAction_None   = 0,
    SlimeAction_Attack = 1,
} SlimeAction;

typedef struct Slime {
    const char *name;
    Body3D body;
    Combat combat;
    SlimeAction action;
    double randJumpIdle;
} Slime;

void slime_init                     (Slime *slime, const char *name, struct Sprite *sprite);
void slime_move                     (Slime *slime, double now, double dt, Vector2 offset);
void slime_combine                  (Slime *slimeA, Slime *slimeB);
bool slime_attack                   (Slime *slime, double now, double dt);
void slime_update                   (Slime *slime, double now, double dt);
void slime_draw                     (Slime *slime);