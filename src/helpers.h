#pragma once
#include "enet_zpl.h"
#include "raylib/raylib.h"
#include "dlb_types.h"
#include <cstdint>
//#include <cstdbool>

//------------------------------------------------------------------------------
// Helper macros
//------------------------------------------------------------------------------
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))
#define LERP(a, b, alpha) ((a) + ((b) - (a)) * (alpha))
#define SQUARED(x) ((x)*(x))

// NOTE: This defines 1 meter = 64 pixels
#define METERS_TO_PIXELS(meters) ((meters) * 64.0f)
#define PIXELS_TO_METERS(pixels) ((pixels) / 64.0f)

#define POSITION_EPSILON 0.0001f
#define VELOCITY_EPSILON 0.001f

//------------------------------------------------------------------------------
// Feature flags
//------------------------------------------------------------------------------
#if _DEBUG
    #define RUN_TESTS 1
#else
    #define RUN_TESTS 0
#endif

#define GF_SKIP_BODY_FLOORF              1
#define GF_LOOT_TABLE_MONTE_CARLO        (0 && _DEBUG)
#define CL_CULL_ON_PUSH                  1
#define CL_CURSOR_ITEM_HIDES_POINTER     0
#define CL_CURSOR_ITEM_RELATIVE_TERRARIA 0
#define CL_CURSOR_ITEM_TEXT_BOTTOM_LEFT  1
#define CL_PIXEL_FIXER                   1
#define CL_SMOOTH_PLAYER_RECONCILIATION  1
#define CL_DEBUG_ADVANCED_ITEM_TOOLTIPS  (1 && _DEBUG)
#define CL_DEBUG_PLAYER_RECONCILIATION   (0 && _DEBUG)
#define CL_DEBUG_SNAPSHOT_INTERPOLATION  (0 && _DEBUG)
#define CL_DEBUG_REALLY_LONG_TIMEOUT     (0 && _DEBUG)
#define CL_DEBUG_SHOW_LEVELS             (0 && _DEBUG)
#define CL_DEBUG_SPEEDHAX                (1 && _DEBUG)
#define CL_DEBUG_WORLD_CHUNKS            (0 && _DEBUG)
#define CL_DEBUG_WORLD_ITEMS             (0 && _DEBUG)
#define CL_DEMO_AI_TRACKING              (0 && _DEBUG)
#define CL_DEMO_BODY_RECT                (0 && _DEBUG)
#define CL_DEMO_SNAPSHOT_RADII           (1 && _DEBUG)
#define CL_DEMO_SPAWN_RADII              (0 && _DEBUG)
#define CL_DEMO_VIEW_RTREE               (0 && _DEBUG)
#define SV_DEBUG_SPAWN_REALLY_CLOSE      (0 && _DEBUG)
#define SV_DEBUG_INPUT_SAMPLES           (0 && _DEBUG)
#define SV_DEBUG_WORLD_CHUNKS            (0 && _DEBUG)
#define SV_DEBUG_WORLD_NPCS              (0 && _DEBUG)
#define SV_DEBUG_WORLD_ITEMS             (0 && _DEBUG)
#define SV_DEBUG_WORLD_PLAYERS           (0 && _DEBUG)

#if _DEBUG
    #define SHOW_DEBUG_STATS 1
#else
    #define SHOW_DEBUG_STATS 1
#endif

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define ITEM_W 32
#define ITEM_H ITEM_W
#define CHUNK_W 16
#define CHUNK_H CHUNK_W
#define TILE_W 32
#define TILE_H TILE_W
#define SUBTILE_W 8
#define SUBTILE_H SUBTILE_W

#define SV_DEFAULT_PORT             4040
#define SV_SINGLEPLAYER_HOST        "localhost"
#define SV_SINGLEPLAYER_PORT        4039
#define SV_SINGLEPLAYER_USER        "guest"
#define SV_SINGLEPLAYER_PASS        "guest"
#define SV_USERNAME                 "SERVER"
#define SV_MAX_PLAYERS              8
#define SV_MAX_NPC_SLIMES           16
#define SV_MAX_NPC_TOWNFOLK         1
#define SV_MAX_NPCS (               SV_MAX_NPC_SLIMES   + \
                                    SV_MAX_NPC_TOWNFOLK )
#define SV_MAX_ITEMS                256 //4096
#define SV_WORLD_ITEM_LIFETIME      120 //600 // despawn items after 10 minutes
#define SV_TICK_RATE                60
#define SV_TICK_DT                  (1.0 / SV_TICK_RATE)
#define SV_TICK_DT_ACCUM_MAX        (1.5 * SV_TICK_DT)
#define SV_TIME_SECONDS_IN_DAY      600.0
#define SV_TIME_WHEN_GAME_STARTS    (SV_TIME_SECONDS_IN_DAY * (1.0 / 24.0) * (11.0 - 1.0))  // start the game at 11 am
#define SV_INPUT_HISTORY            SV_TICK_RATE
#define SV_INPUT_HISTORY_DT_MAX     1.0  //(5.0 * SV_TICK_DT)  // discard buffered inputs that exceed a sane dt accumulation
#define SV_WORLD_HISTORY            SV_TICK_RATE
#define SV_TILE_UPDATE_DIST         METERS_TO_PIXELS(20.0f)
// NOTE: max diagonal distance at 1080p is 1100 + radius units. 1200px allows for a ~50px wide entity
#if SV_DEBUG_SPAWN_REALLY_CLOSE
#define SV_PLAYER_NEARBY_THRESHOLD  300.0f                       // how close a player has to be to appear in your snapshot
#define SV_NPC_NEARBY_THRESHOLD     METERS_TO_PIXELS(6.0f)       // how close an NPC has to be to appear in your snapshot
#define SV_ENEMY_MIN_SPAWN_DIST     METERS_TO_PIXELS(8.0f)       // closest enemies can spawn to a player
#define SV_ENEMY_DESPAWN_RADIUS     METERS_TO_PIXELS(10.0f)      // furthest enemies can be from a player before despawning
#define SV_ITEM_NEARBY_THRESHOLD    300.0f                       // how close an item has to be to receive a snapshot
#else
#define SV_PLAYER_NEARBY_THRESHOLD  METERS_TO_PIXELS(20.0f)      // how close a player has to be to appear in your snapshot
#define SV_NPC_NEARBY_THRESHOLD     METERS_TO_PIXELS(20.0f)      // how close an NPC has to be to appear in your snapshot
#define SV_ENEMY_MIN_SPAWN_DIST     METERS_TO_PIXELS(25.0f)      // closest enemies can spawn to a player
#define SV_ENEMY_DESPAWN_RADIUS     METERS_TO_PIXELS(40.0f)      // furthest enemies can be from a player before despawning
#define SV_ITEM_NEARBY_THRESHOLD    METERS_TO_PIXELS(20.0f)      // how close an item has to be to receive a snapshot
#endif
#define SV_ITEM_ATTRACT_DIST        METERS_TO_PIXELS(1.0f)       // how close player should be to item to attract it
#define SV_ITEM_PICKUP_DIST         METERS_TO_PIXELS(0.3f)       // how close player should be to item to pick it up
#define SV_ITEM_PICKUP_DELAY        1.0                          // how long after an item is spawned before it can be picked up by a player
#define SV_ITEM_REPICKUP_DELAY      2.0                          // how long after an item is dropped by a player before it can be picked up by the same player
#define SV_PLAYER_MOVE_SPEED        3.0f                         // how fast player walks, in meters
#define SV_PLAYER_ATTACK_COOLDOWN   0.5                          // how often the player can attack
#define SV_PLAYER_CORPSE_LIFETIME   8.0                          // how long to wait after a player dies to despawn their corpse
#define SV_NPC_DESPAWN_LIFETIME     1.0                          // how long to keep manually despawned enemies around give give nearby clients time to be notified
#define SV_NPC_CORPSE_LIFETIME      2.0                          // how long to wait after an NPC dies to despawn their corpse
#define CL_NPC_CORPSE_LIFETIME      (SV_NPC_CORPSE_LIFETIME * 1) // how long to wait after an NPC dies to despawn their corpse (on client, if despawn flag not received)
#define CL_NPC_STALE_LIFETIME       1.0                          // how long to wait to receive the next snapshot before drawing stale marker
#define SV_RESPAWN_TIMER            5.0                          // how long to make player stare at nothing before respawning
#define SV_COMMAND_MAX_ARGS         16                           // max # of args a chat command can have
#define SV_SLIME_MOVE_SPEED         2.0f                         // how fast slimes can move (i.e. jump)
#define SV_SLIME_ATTACK_TRACK       METERS_TO_PIXELS(10.0f)      // how far away slimes can see players
#define SV_SLIME_ATTACK_REACH       METERS_TO_PIXELS(0.5f)       // how far away slimes can reach to attack a player
#define SV_SLIME_RADIUS             METERS_TO_PIXELS(0.5f)       // how thicc a slime is
#define SLIME_MAX_SCALE             3.0f                         // how phat a slime can get
// NOTE: Have legit clients d/c if their FPS drops below 15 fps to prevent them from being banned for hacking due to input latency
#define SV_INPUT_HACK_THRESHOLD     (SV_TICK_DT * 5.0)  // 4 frames of overflowed input time is surely a hacker (or a client with < 15 fps?)

// NOTE: Due to how "enemy.moved" flag is calculated atm, this *MUST* match SV_TICK_RATE
#define SNAPSHOT_SEND_RATE            30  //SV_TICK_RATE  //MIN(30, SV_TICK_RATE)
#define SNAPSHOT_SEND_DT              (1.0 / SV_TICK_RATE)
#define SNAPSHOT_MAX_PLAYERS          SV_MAX_PLAYERS
#define SNAPSHOT_MAX_NPCS             MIN(64, SV_MAX_NPCS)
#define SNAPSHOT_MAX_ITEMS            MIN(64, SV_MAX_ITEMS)

#define CL_FRAME_DT_MAX               (2.0 * SV_TICK_DT)
#define CL_INPUT_SEND_RATE_LIMIT      60 // max # of input packets to sender to server per second
#define CL_INPUT_SEND_RATE_LIMIT_DT   (1.0 / CL_INPUT_SEND_RATE_LIMIT)
#define CL_INPUT_HISTORY              (256) // how many samples to keep around client side
#define CL_INPUT_SAMPLES_MAX          (CL_INPUT_HISTORY) // send up to 1 second of samples per packet
#define CL_WORLD_HISTORY              (SV_TICK_RATE / 2 + 1)  // >= 500 ms of data
#define CL_CHAT_HISTORY               256
#define CL_INVENTORY_UPDATE_SLOTS_MAX 256
#define CL_MAX_PLAYER_POS_DESYNC_DIST METERS_TO_PIXELS(0.01)  // less than 1 pixel delta allowed
#define CL_DAY_NIGHT_CYCLE            0

//#define PACKET_SIZE_MAX         1024
#define PACKET_SIZE_MAX         16384
// Min/max ASCII value for username/password/motd/message, etc.
#define STRING_ASCII_MIN        32
#define STRING_ASCII_MAX        126
#define SERV_DESC_LENGTH_MIN    0
#define SERV_DESC_LENGTH_MAX    64
#define HOSTNAME_LENGTH_MIN     3
#define HOSTNAME_LENGTH_MAX     256
#define USERNAME_LENGTH_MIN     2
#define USERNAME_LENGTH_MAX     32
#define PASSWORD_LENGTH_MIN     4   // TODO: Increase to 8
#define PASSWORD_LENGTH_MAX     64
#define ENTITY_NAME_LENGTH_MAX  64
#define TIMESTAMP_LENGTH        12  // hh:MM:SS AM
#define MOTD_LENGTH_MIN         0
#define MOTD_LENGTH_MAX         64
#define CHATMSG_LENGTH_MIN      0
#define CHATMSG_LENGTH_MAX      512  // not including nil terminator, must be < CHATMSG_BUFFER_LEN
#define WORLD_CHUNK_MIN         INT16_MIN
#define WORLD_CHUNK_MAX         INT16_MAX
#define WORLD_CHUNK_TILES       256
#define ENTITY_POSITION_X_MIN   0
#define ENTITY_POSITION_X_MAX   UINT32_MAX  // Actually a float, so we need to allow full range
#define ENTITY_POSITION_Y_MIN   0
#define ENTITY_POSITION_Y_MAX   UINT32_MAX  // Actually a float, so we need to allow full range

#define CHAT_MAX_MSG_COUNT      20
#define CHAT_BG_ALPHA           0.5f

// Type alises
typedef uint32_t ChunkHash;
typedef uint8_t  ItemSlot;
typedef uint8_t  ItemClass;
typedef uint16_t ItemType;
typedef uint8_t  ItemAffixType;
typedef uint32_t ItemUID;
typedef uint8_t  SlotId;

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
thread_local static Shader  g_sdfShader                  {};
thread_local static uint8_t g_inputMsecHax               {};
thread_local static bool    g_cl_client_prediction       = true;
thread_local static bool    g_cl_smooth_reconcile        = true;
thread_local static float   g_cl_smooth_reconcile_factor = 0.3f;
thread_local static bool    g_cl_show_snapshot_shadow    = false;
thread_local static Texture g_nPatchTex                  {};

void DrawTextFont(Font font, const char *text, float posX, float posY, float offsetX, float offsetY, int fontSize, const Color &color);
const char *SafeTextFormat(const char *text, ...);
const char *SafeTextFormatIP(ENetAddress &address);
const char *SafeTextFormatTimestamp();
float VolumeCorrection(float volume);
bool PointInRect(Vector2 &point, Rectangle &rect);
