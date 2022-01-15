#pragma once
#include "body.h"
#include "sprite.h"
#include "raylib/raylib.h"

struct HealthBar {
    static void SetFont(const Font font);
    static void Draw(int fontSize, const Sprite &sprite, const Body3D &body, const char *name, int hitPoints, int maxHitPoints);

private:
    static Font s_font;
};