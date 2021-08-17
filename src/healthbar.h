#pragma once
#include "body.h"
#include "sprite.h"
#include "raylib.h"

struct HealthBar {
    static void SetFont(const Font font);
    static void Draw(int fontSize, const Sprite &sprite, const Body3D &body, float hitPoints, float maxHitPoints);

private:
    static Font s_font;
};