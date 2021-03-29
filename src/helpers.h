#pragma once
#include "raylib.h"
#include "dlb_types.h"
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Feature flags
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define PACKET_SIZE_MAX     1024
#define USERNAME_LENGTH_MAX 32

// must be power of 2 (shift modulus ring buffer)
#define CHAT_MESSAGE_HISTORY    64
#define CHAT_MESSAGE_LENGTH_MAX 500  // not including nil terminator, must be < CHAT_MESSAGE_BUFFER_LEN
#define CHAT_MESSAGE_BUFFER_LEN 512  // including nil terminator

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))
#define SQUARED(x) ((x)*(x))

// NOTE: This defines 1 meter = 64 pixels
#define METERS_TO_PIXELS(meters) ((meters) * 64.0f)
#define PIXELS_TO_METERS(pixels) ((pixels) / 64.0f)

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
void DrawTextFont(Font font, const char *text, float posX, float posY, int fontSize, Color color);
