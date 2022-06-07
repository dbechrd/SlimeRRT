#pragma once

#include "raylib/raylib.h"
#include "imgui/imgui.h"

struct Fonts {
    ImFont *imFontHack16{};
    ImFont *imFontHack32{};
    ImFont *imFontHack48{};
    ImFont *imFontHack64{};
    Font fontSmall{};
    Font fontBig{};
    Font fontSdf24{};
    Font fontSdf72{};
} g_fonts;
