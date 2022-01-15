#pragma once
#include "body.h"
#include "sprite.h"
#include "raylib/raylib.h"

struct HealthBar {
    static void SetFont(const Font font);
    static void Draw(int fontSize, const Vector3i &topCenter, uint32_t id, const char *name, int hitPoints, int maxHitPoints);

private:
    static Font s_font;
};