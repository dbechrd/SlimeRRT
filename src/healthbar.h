#pragma once
#include "body.h"
#include "combat.h"
#include "sprite.h"
#include "raylib/raylib.h"

struct HealthBar {
    static void SetFont(const Font font);
    static void Draw(const Vector2 &topCenter, const char *name, const Combat &combat, EntityID entityId);
    static void Dialog(const Vector2 &topCenter, const char *text);

private:
    static Font s_font;
};