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

// TODO: This could go in direction.h.. but that seems unnecessary atm
typedef enum Direction {
    Direction_North     = 0,  // 000
    Direction_East      = 1,  // 001
    Direction_South     = 2,  // 010
    Direction_West      = 3,  // 011
    Direction_NorthEast = 4,  // 100
    Direction_SouthEast = 5,  // 101
    Direction_SouthWest = 6,  // 110
    Direction_NorthWest = 7,  // 111
    Direction_Count
} Direction;

typedef struct StringView {
    size_t length;              // length of string (without nil terminator, these strings are views into a larger buffer)
    const unsigned char *text;  // pointer into someone else's buffer, do not attempt to free
} StringView;

void DrawTextFont(Font font, const char *text, int posX, int posY, int fontSize, Color color);