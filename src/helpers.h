#pragma once
#include "raylib/raylib.h"
#include "dlb_types.h"
#include <cstdint>
//#include <cstdbool>

//------------------------------------------------------------------------------
// Feature flags
//------------------------------------------------------------------------------
#define CULL_ON_PUSH 1
#define DEMO_VIEW_RTREE 0
#define DEMO_VIEW_CULLING 0
#define DEMO_AI_TRACKING 0
#define DEMO_BODY_RECT 0

#if _DEBUG
    #define SHOW_DEBUG_STATS 1
#else
    #define SHOW_DEBUG_STATS 1
#endif

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define SERVER_PORT         4040
#define SERVER_USERNAME     "SERVER"
#define PACKET_SIZE_MAX     1024
#define USERNAME_LENGTH_MAX 32
#define PASSWORD_LENGTH_MAX 32

#define CHAT_MESSAGE_HISTORY    16 //64
#define CHAT_MESSAGE_LENGTH_MAX 500  // not including nil terminator, must be < CHAT_MESSAGE_BUFFER_LEN
#define CHAT_MESSAGE_BUFFER_LEN 512  // including nil terminator

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))
#define LERP(a, b, alpha) ((a) * (1.0f - (alpha)) + b * (alpha))
#define SQUARED(x) ((x)*(x))

// NOTE: This defines 1 meter = 64 pixels
#define METERS_TO_PIXELS(meters) ((meters) * 64.0f)
#define PIXELS_TO_METERS(pixels) ((pixels) / 64.0f)

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
void DrawTextFont(Font font, const char *text, float posX, float posY, int fontSize, Color color);
