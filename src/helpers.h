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
#define SERVER_HOST_LENGTH_MAX  64
#define SERVER_PORT             4040
//#define PACKET_SIZE_MAX        1024
#define SERVER_USERNAME         "SERVER"
#define SERVER_MAX_PLAYERS      8

#define PACKET_SIZE_MAX         (UINT16_MAX * 2)

// Min/max ASCII value for username/password/motd/message, etc.
#define STRING_ASCII_MIN        32
#define STRING_ASCII_MAX        126
#define USERNAME_LENGTH_MIN     2
#define USERNAME_LENGTH_MAX     32
#define PASSWORD_LENGTH_MIN     0  // TODO: Increase to 8
#define PASSWORD_LENGTH_MAX     64
#define MOTD_LENGTH_MIN         0
#define MOTD_LENGTH_MAX         64
#define WORLD_WIDTH_MIN         1
#define WORLD_WIDTH_MAX         256
#define WORLD_HEIGHT_MIN        1
#define WORLD_HEIGHT_MAX        256
#define WORLD_CHUNK_WIDTH       16
#define WORLD_CHUNK_HEIGHT      16
#define WORLD_MAX_TILES         65536
#define WORLD_MAX_ENTITIES      256
#define ENTITY_POSITION_X_MIN   0
#define ENTITY_POSITION_X_MAX   UINT32_MAX
#define ENTITY_POSITION_Y_MIN   0
#define ENTITY_POSITION_Y_MAX   UINT32_MAX

#define CHAT_MESSAGE_HISTORY    16
#define CHAT_MESSAGE_LENGTH_MIN 0
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
const char *TextFormatIP(ENetAddress address);