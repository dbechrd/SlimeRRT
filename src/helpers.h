#pragma once
#include "enet_zpl.h"
#include "raylib/raylib.h"
#include "dlb_types.h"
#include <cstdint>
//#include <cstdbool>

//------------------------------------------------------------------------------
// Feature flags
//------------------------------------------------------------------------------
#define CULL_ON_PUSH 1
#define DEMO_VIEW_RTREE 0
#define DEMO_AI_TRACKING 0
#define DEMO_BODY_RECT 0
#define SV_DEBUG_INPUT 0

#if _DEBUG
    #define SHOW_DEBUG_STATS 1
#else
    #define SHOW_DEBUG_STATS 1
#endif

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define SV_HOSTNAME_LENGTH_MAX  256
#define SV_DEFAULT_PORT         4040
#define SV_SINGLEPLAYER_HOST    "localhost"
#define SV_SINGLEPLAYER_PORT    4039
#define SV_SINGLEPLAYER_USER    "user"
#define SV_SINGLEPLAYER_PASS    "pass"
#define SV_USERNAME             "SERVER"
#define SV_MAX_PLAYERS          8
#define SV_MAX_SLIMES           256
#define SV_MAX_ITEMS            256 //4096
#define SV_WORLD_ITEM_LIFETIME  16 //600 // despawn items after 10 minutes
#define SV_TICK_RATE            60
#define SV_INPUT_HISTORY        (SV_TICK_RATE * SV_MAX_PLAYERS)
#define SV_WORLD_HISTORY        SV_TICK_RATE

// NOTE: max diagonal distance at 1080p is 1100 + radius units. 1200px allows for a ~50px wide entity
#define SV_PLAYER_NEARBY_THRESHOLD 1200
#define SV_ENEMY_NEARBY_THRESHOLD  1200

// NOTE: Due to how "enemy.moved" flag is calculated atm, this *MUST* match SV_TICK_RATE
#define SNAPSHOT_SEND_RATE      SV_TICK_RATE  //30 //MIN(20, SV_TICK_RATE)
#define SNAPSHOT_MAX_PLAYERS    SV_MAX_PLAYERS
#define SNAPSHOT_MAX_SLIMES     MIN(64, SV_MAX_SLIMES)
#define SNAPSHOT_MAX_ITEMS      MIN(64, SV_MAX_ITEMS)

#define CL_INPUT_SAMPLE_RATE    SV_TICK_RATE  // must be equal to SV_TICK_RATE
#define CL_INPUT_SEND_RATE      SV_TICK_RATE  // can be <= CL_INPUT_SAMPLE_RATE
#define CL_INPUT_SAMPLES_MAX    SV_TICK_RATE  // send up to 1 second of samples per packet
#define CL_INPUT_HISTORY        (SV_TICK_RATE * 4)  // keep 4 seconds of input data
#define CL_WORLD_HISTORY        (SV_TICK_RATE / 2 + 1)  // >= 500 ms of data
#define CL_CHAT_HISTORY         256

#define PACKET_SIZE_MAX         1024
//#define PACKET_SIZE_MAX         16384

// Min/max ASCII value for username/password/motd/message, etc.
#define STRING_ASCII_MIN        32
#define STRING_ASCII_MAX        126
#define USERNAME_LENGTH_MIN     2
#define USERNAME_LENGTH_MAX     32
#define PASSWORD_LENGTH_MIN     4   // TODO: Increase to 8
#define PASSWORD_LENGTH_MAX     64
#define TIMESTAMP_LENGTH        12  // hh:MM:SS AM
#define MOTD_LENGTH_MIN         0
#define MOTD_LENGTH_MAX         64
#define CHATMSG_LENGTH_MIN      0
#define CHATMSG_LENGTH_MAX      512  // not including nil terminator, must be < CHATMSG_BUFFER_LEN
#define WORLD_WIDTH_MIN         1
#define WORLD_WIDTH_MAX         256
#define WORLD_HEIGHT_MIN        1
#define WORLD_HEIGHT_MAX        256
#define WORLD_CHUNK_WIDTH       16
#define WORLD_CHUNK_HEIGHT      16
#define WORLD_CHUNK_TILES_MAX   256
#define ENTITY_POSITION_X_MIN   0
#define ENTITY_POSITION_X_MAX   UINT32_MAX  // Actually a float, so we need to allow full range
#define ENTITY_POSITION_Y_MIN   0
#define ENTITY_POSITION_Y_MAX   UINT32_MAX  // Actually a float, so we need to allow full range

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))
#define LERP(a, b, alpha) ((a) * (1.0f - (alpha)) + b * (alpha))
#define SQUARED(x) ((x)*(x))

// NOTE: This defines 1 meter = 64 pixels
#define METERS_TO_PIXELS(meters) ((meters) * 64)
#define PIXELS_TO_METERS(pixels) ((pixels) / 64)

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
extern Shader g_sdfShader;

void DrawTextFont(Font font, const char *text, int posX, int posY, int fontSize, const Color &color);
const char *TextFormatIP(ENetAddress &address);
const char *TextFormatTimestamp();
float VolumeCorrection(float volume);