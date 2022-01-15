#pragma once
#include "body.h"
#include "sprite.h"
#include "raylib/raylib.h"

struct HealthBar {
    static void SetFont(const Font font);
    static void Draw(int fontSize, const Vector2 &topCenter, const char *name, float hitPoints, float maxHitPoints);

private:
    static Font s_font;
};