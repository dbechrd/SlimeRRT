#pragma once
#include "body.h"
#include "sprite.h"
#include "raylib.h"

void healthbars_set_font (const Font font);
void healthbar_draw      (int fontSize, const Sprite *sprite, const Body3D *body, float hitPoints, float maxHitPoints);