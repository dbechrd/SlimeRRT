#pragma once
#include "raylib.h"
#include "dlb_types.h"
#include <stdint.h>
#include <stdbool.h>

#define CULL_ON_PUSH 1
#define DEMO_VIEW_CULLING 0
#define DEMO_AI_TRACKING 0
#define DEMO_BODY_RECT 0

#if _DEBUG
    #define ALPHA_NETWORKING 1
    #define SHOW_DEBUG_STATS 1
#else
    #define ALPHA_NETWORKING 0
    #define SHOW_DEBUG_STATS 1
#endif

#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))
#define SQUARED(x) ((x)*(x))

// NOTE: This defines 1 meter = 64 pixels
#define METERS_TO_PIXELS(meters) ((meters) * 64.0f)
#define PIXELS_TO_METERS(pixels) ((pixels) / 64.0f)

void DrawTextFont(Font font, const char *text, float posX, float posY, int fontSize, Color color);
