#pragma once
#include "raylib.h"
#include "dlb_types.h"
#include <stdint.h>
#include <stdbool.h>

#define DEMO_VIEW_CULLING 0
#define DEMO_AI_TRACKING 0
#define DEMO_BODY_RECT 0

#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))
#define SQUARED(x) ((x)*(x))

// NOTE: This defines 1 meter = 64 pixels
#define METERS(m) ((m) * 64.0f)

// TODO: This could go in facing.h.. but that seems unnecessary atm
typedef enum Facing {
    Facing_Idle,
    Facing_North,
    Facing_East,
    Facing_South,
    Facing_West,
    Facing_NorthEast,
    Facing_SouthEast,
    Facing_SouthWest,
    Facing_NorthWest,
    Facing_Count
} Facing;

typedef struct StringView {
    size_t length;              // length of string (without nil terminator, these strings are views into a larger buffer)
    const unsigned char *text;  // pointer into someone else's buffer, do not attempt to free
} StringView;

void DrawTextFont(Font font, const char *text, int posX, int posY, int fontSize, Color color);