#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <array>
#include <thread>

#include "bit_stream.h"
#include "chat.h"
#include "controller.h"
#include "draw_command.h"
#include "healthbar.h"
#include "helpers.h"
#include "item_catalog.h"
#include "maths.h"
#include "network_client.h"
#include "network_server.h"
#include "particles.h"
#include "particle_fx.h"
#include "player.h"
#include "slime.h"
#include "sound_catalog.h"
#include "spritesheet.h"
#include "spritesheet_catalog.h"
#include "tileset.h"
#include "tilemap.h"
#include "rstar.h"
#include "sim.h"
#include "world.h"

#include "dlb_rand.h"
#include "raygui.h"
#include "gui_textbox_extended.h"
#include "raylib.h"
#include "zed_net.h"

static FILE *logFile;

void traceLogCallback(int logType, const char *text, va_list args)
{
    vfprintf(logFile, text, args);
    fputs("\n", logFile);
    vfprintf(stdout, text, args);
    fputs("\n", stdout);
    fflush(logFile);
    if (logType >= LOG_WARNING) {
        assert(!"Catch in debugger");
        exit(-1);
    }
}

Rectangle RectPad(const Rectangle rec, float pad)
{
    Rectangle padded = { rec.x - pad, rec.y - pad, rec.width + pad * 2.0f, rec.height + pad * 2.0f };
    return padded;
}

Rectangle RectPadXY(const Rectangle rec, float padX, float padY)
{
    Rectangle padded = { rec.x - padX, rec.y - padY, rec.width + padX * 2.0f, rec.height + padY * 2.0f };
    return padded;
}

#define DLB_RAND_TEST
#include "dlb_rand.h"

static NetworkServer server = {};
void network_server_thread()
{
    if (!network_server_init(&server)) {
        TraceLog(LOG_FATAL, "Failed to initialize network server system.\n");
    }

    const unsigned int SERVER_PORT = 4040;
    if (!network_server_open_socket(&server, SERVER_PORT)) {
        TraceLog(LOG_ERROR, "Failed to open server socket on port %hu.\n", SERVER_PORT);
        // TODO: Handle gracefully if local server to allow singleplayer mode
        exit(-1);
    }

    network_server_receive(&server);

    network_server_free(&server);
}

struct Args {
    bool server;
};

void parse_args(Args &args, int argc, char *argv[])
{
    for (int i = 0; i < argc; i++) {
        printf("args[%d] = '%s'\n", i, argv[i]);

        if (!strcmp(argv[i], "-s")) {
            args.server = true;
            // TODO: Check if next arg is a port
        }
    }
}

int main(int argc, char *argv[])
{
    Args args = {};
    parse_args(args, argc, argv);

    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    logFile = fopen("log.txt", "w");
    SetTraceLogCallback(traceLogCallback);

    World world = {};

    world.rtt_seed = time(NULL);
    dlb_rand32_seed_r(&world.rtt_rand, world.rtt_seed, world.rtt_seed);

    //dlb_rand_test();

#if 0
    // TODO: bit_stream.h with tests
    {
        bit_stream_write(stream, 0b0, 1);
        bit_stream_write(stream, 0b11, 2);
        bit_stream_write(stream, 0b000, 3);
        bit_stream_write(stream, 0b1111, 4);
        bit_stream_write(stream, 0b00000, 5);
        bit_stream_write(stream, 0b111111, 6);
        bit_stream_write(stream, 0b0000000, 7);
        bit_stream_write(stream, 0b11111111, 8);
        bit_stream_flush(stream);

        assert(bit_stream_read(stream, 1) == 0b0       );
        assert(bit_stream_read(stream, 2) == 0b11      );
        assert(bit_stream_read(stream, 3) == 0b000     );
        assert(bit_stream_read(stream, 4) == 0b1111    );
        assert(bit_stream_read(stream, 5) == 0b00000   );
        assert(bit_stream_read(stream, 6) == 0b111111  );
        assert(bit_stream_read(stream, 7) == 0b0000000 );
        assert(bit_stream_read(stream, 8) == 0b11111111);
    }
#endif
    {
        NetMessage msgWritten = {};
        msgWritten.type = NetMessageType_ChatMessage;
        msgWritten.data.chatMessage.username = "dandymcgee";
        msgWritten.data.chatMessage.usernameLength = strlen(msgWritten.data.chatMessage.username);
        msgWritten.data.chatMessage.message = "This is a test message";
        msgWritten.data.chatMessage.messageLength = strlen(msgWritten.data.chatMessage.message);

        char rawPacket[PACKET_SIZE_MAX] = {};
        BitStream chatWriter = {};
        bit_stream_writer_init(&chatWriter, (uint32_t *)rawPacket, sizeof(rawPacket));
        serialize_net_message(&chatWriter, &msgWritten);

        NetMessage msgRead = {};
        BitStream chatReader = {};
        assert(chatWriter.total_bits % 8 == 0);
        bit_stream_reader_init(&chatReader, (uint32_t *)rawPacket, chatWriter.total_bits / 8);
        deserialize_net_message(&chatReader, &msgRead);

        assert(msgRead.type == msgWritten.type);
        assert(msgRead.data.chatMessage.usernameLength == msgWritten.data.chatMessage.usernameLength);
        assert(!strncmp(msgRead.data.chatMessage.username, msgWritten.data.chatMessage.username, msgRead.data.chatMessage.usernameLength));
        assert(msgRead.data.chatMessage.messageLength == msgWritten.data.chatMessage.messageLength);
        assert(!strncmp(msgRead.data.chatMessage.message, msgWritten.data.chatMessage.message, msgRead.data.chatMessage.messageLength));
    }

    ////////////////////////////////////////////////////////////////////////////

    const char *title = "Attack the slimes!";
    if (args.server) {
        title = "[Server] Attack the slimes!";
    }
    InitWindow(1600, 900, title);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);  // Disable default Escape exit key, we'll handle escape ourselves

#if ALPHA_NETWORKING
    if (zed_net_init() < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_FATAL, "Failed to initialize network utilities. Error: %s\n", err);
    }

    std::thread server_thread = {};
    if (args.server) {
        server_thread = std::thread(network_server_thread);
    }

    NetworkClient client = {};
    if (!network_client_init(&client)) {
        TraceLog(LOG_FATAL, "Failed to initialize network client system.\n");
    }

    if (!network_client_open_socket(&client)) {
        TraceLog(LOG_ERROR, "Failed to open client socket.\n");
        // TODO: Handle "offline mode" gracefully. This shouldn't prevent people people from playing single player
        exit(-1);
    }

    //if (!network_client_connect(&client, "slime.theprogrammingjunkie.com", 4040)) {
    if (!network_client_connect(&client, "127.0.0.1", 4040)) {
        // TODO: Handle connection failure.
        TraceLog(LOG_FATAL, "Failed to connect client.\n");
    }
#endif

    const int fontHeight = 14;
    Font fonts[3] = {
        GetFontDefault(),
        LoadFontEx("C:/Windows/Fonts/consola.ttf", fontHeight, 0, 0),
        LoadFontEx("resources/UbuntuMono-Regular.ttf", fontHeight, 0, 0),
    };
    for (size_t i = 0; i < ARRAY_SIZE(fonts); i++) {
        assert(fonts[i].texture.id);
    }
    size_t fontIdx = 1;
    GuiSetFont(fonts[fontIdx]);

    healthbars_set_font(fonts[0]);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("ERROR: Failed to initialized audio device\n");
    }
    // NOTE: Minimum of 0.001 seems reasonable (0.0001 is still audible on max volume)
    SetMasterVolume(0.01f);

    draw_commands_init();
    sound_catalog_init();
    spritesheet_catalog_init();
    item_catalog_init();
    particles_init();

    Music mus_background;
    mus_background = LoadMusicStream("resources/fluquor_copyright.ogg");
    mus_background.looping = true;
    PlayMusicStream(mus_background);
    SetMusicVolume(mus_background, 0.02f);
    UpdateMusicStream(mus_background);

    Music mus_whistle = LoadMusicStream("resources/whistle.ogg");
    mus_whistle.looping = true;

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    Texture tilesetTex = LoadTexture("resources/tiles32.png");
    assert(tilesetTex.width);

    Tileset tileset = {};
    tileset_init_ex(&tileset, &tilesetTex, 32, 32, TileType_Count);

    Tilemap tilemap = {};
    //tilemap_generate_ex(&tilemap, 128, 128, &tileset);
    tilemap_generate_ex(&tilemap, 256, 256, &tileset);
    //tilemap_generate_ex(&tilemap, 512, 512, &tileset);

    Texture minimapTex = {};
    {
        Image minimapImg = {};
        minimapImg.width = (int)tilemap.widthTiles;
        minimapImg.height = (int)tilemap.heightTiles;
        minimapImg.mipmaps = 1;
        minimapImg.format = UNCOMPRESSED_R8G8B8A8;
        assert(sizeof(Color) == 4);
        minimapImg.data = calloc((size_t)minimapImg.width * minimapImg.height, sizeof(Color));
        assert(minimapImg.data);

        Color tileColors[TileType_Count]{};
        tileColors[TileType_Grass   ] = GREEN;
        tileColors[TileType_Water   ] = SKYBLUE;
        tileColors[TileType_Forest  ] = DARKGREEN;
        tileColors[TileType_Wood    ] = BROWN;
        tileColors[TileType_Concrete] = GRAY;

        const size_t heightTiles = tilemap.heightTiles;
        const size_t widthTiles = tilemap.widthTiles;

        Color *minimapPixel = (Color *)minimapImg.data;
        for (size_t y = 0; y < heightTiles; y += 1) {
            for (size_t x = 0; x < widthTiles; x += 1) {
                const Tile *tile = &tilemap.tiles[y * tilemap.widthTiles + x];
                // Draw all tiles as different colored pixels
                assert(tile->tileType >= 0);
                assert(tile->tileType < TileType_Count);
                *minimapPixel = tileColors[tile->tileType];
                minimapPixel++;
            }
        }

        minimapTex = LoadTextureFromImage(minimapImg);
        free(minimapImg.data);
    }

    const Vector3 worldSpawn = {
        (float)tilemap.widthTiles / 2.0f * tilemap.tileset->tileWidth,
        (float)tilemap.heightTiles / 2.0f * tilemap.tileset->tileHeight,
        0.0f
    };

#if DEMO_VIEW_RSTAR
    std::array<Rectangle, 100> rects{};
    std::array<bool, 100> drawn{};
    RStar::RStarTree<int> tree{};

    dlb_rand32_t rstar_rand{};
    dlb_rand32_seed_r(&rstar_rand, 42, 42);
    for (size_t i = 0; i < rects.size(); i++) {
        rects[i].x = worldSpawn.x + dlb_rand32f_variance_r(&rstar_rand, 400.0f);
        rects[i].y = worldSpawn.y + dlb_rand32f_variance_r(&rstar_rand, 400.0f);
        rects[i].width  = dlb_rand32f_range_r(&rstar_rand, 10.0f, 50.0f);
        rects[i].height = dlb_rand32f_range_r(&rstar_rand, 10.0f, 50.0f);
    }
#endif

    Camera2D camera = {};
    //camera.target = (Vector2){
    //    (float)tilemap.widthTiles / 2.0f * tilemap.tileset->tileWidth,
    //    (float)tilemap.heightTiles / 2.0f * tilemap.tileset->tileHeight
    //};
    camera.offset.x = screenWidth / 2.0f;
    camera.offset.y = screenHeight / 2.0f;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    bool cameraReset = false;
    bool cameraFollowPlayer = true;

    // TODO: Move sprite loading to somewhere more sane
    const Spritesheet *charlieSpritesheet = spritesheet_catalog_find(SpritesheetID_Charlie);
    const SpriteDef *charlieSpriteDef = spritesheet_find_sprite(charlieSpritesheet, "player_sword");

    const Spritesheet *slimeSpritesheet = spritesheet_catalog_find(SpritesheetID_Slime);
    const SpriteDef *slimeSpriteDef = spritesheet_find_sprite(slimeSpritesheet, "slime");

    const Spritesheet *coinSpritesheet = spritesheet_catalog_find(SpritesheetID_Coin);
    const SpriteDef *coinSpriteDef = spritesheet_find_sprite(coinSpritesheet, "coin");

    Player charlie = {};
    player_init(&charlie, "Charlie", charlieSpriteDef);
    charlie.body.position = worldSpawn;

    Slime slimes[MAX_SLIMES] = {};

    {
        // TODO: Slime radius should probably be determined base on largest frame, no an arbitrary frame. Or, it could
        // be specified in the config file.
        assert(slimeSpriteDef);
        int firstAnimIndex = slimeSpriteDef->animations[0];
        assert(firstAnimIndex >= 0);
        assert(firstAnimIndex < slimeSpriteDef->spritesheet->animationCount);

        int firstFrameIdx = slimeSpriteDef->spritesheet->animations[firstAnimIndex].frames[0];
        assert(firstFrameIdx >= 0);
        assert(firstFrameIdx < slimeSpriteDef->spritesheet->frameCount);

        SpriteFrame *firstFrame = &slimeSpritesheet->frames[firstFrameIdx];
        const float slimeRadiusX = firstFrame->width / 2.0f;
        const float slimeRadiusY = firstFrame->height / 2.0f;
        const size_t mapPixelsX = tilemap.widthTiles * tilemap.tileset->tileWidth;
        const size_t mapPixelsY = tilemap.heightTiles * tilemap.tileset->tileHeight;
        const float maxX = mapPixelsX - slimeRadiusX;
        const float maxY = mapPixelsY - slimeRadiusY;
        for (int i = 0; i < MAX_SLIMES; i++) {
            slime_init(&slimes[i], 0, slimeSpriteDef);
            slimes[i].body.position.x = dlb_rand32f_range(slimeRadiusX, maxX);
            slimes[i].body.position.y = dlb_rand32f_range(slimeRadiusY, maxY);
        }
    }

    double frameStart = GetTime();
    double dt = 0;
    const int targetFPS = 60;
    //SetTargetFPS(targetFPS);
    SetWindowState(FLAG_VSYNC_HINT);
    bool gifRecording = false;
    bool chatActive = false;
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        bool escape = IsKeyPressed(KEY_ESCAPE);
        bool findMouseTile = false;

        // NOTE: Limit delta time to 2 frames worth of updates to prevent chaos for large dt (e.g. when debugging)
        const double dtMax = (1.0 / targetFPS) * 2;
        const double now = GetTime();
        dt = MIN(now - frameStart, dtMax);
        frameStart = now;

        // HACK: No way to check if Raylib is currently recording.. :(
        if (gifRecording) {
            dt = 1.0 / 10.0;
        }

#if ALPHA_NETWORKING
        network_client_receive(&client);
#endif

        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            cameraReset = 1;
        }

        UpdateMusicStream(mus_background);
        UpdateMusicStream(mus_whistle);

        if (!chatActive) {
            findMouseTile = IsKeyDown(KEY_LEFT_ALT);

            if (IsKeyPressed(KEY_F11)) {
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                char screenshotName[64] = {};
                int len = snprintf(screenshotName, sizeof(screenshotName),
                    "screenshots/%d-%02d-%02d_%02d-%02d-%02d_screenshot.png",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                assert(len < sizeof(screenshotName));
                TakeScreenshot(screenshotName);
            }

            if (IsKeyPressed(KEY_SEVEN)) {
                fontIdx++;
                if (fontIdx >= ARRAY_SIZE(fonts)) {
                    fontIdx = 0;
                    GuiSetFont(fonts[fontIdx]);
                }
            }

            if (IsKeyPressed(KEY_V)) {
                IsWindowState(FLAG_VSYNC_HINT) ? ClearWindowState(FLAG_VSYNC_HINT) : SetWindowState(FLAG_VSYNC_HINT);
            }

#if ALPHA_NETWORKING
            if (IsKeyPressed(KEY_C)) {
                network_client_send_chat_message(&client, CSTR("User pressed the C key."));
            }
#endif

            if (IsKeyPressed(KEY_F)) {
                cameraFollowPlayer = !cameraFollowPlayer;
            }

            // Camera reset (zoom and rotation)
            if (cameraReset || IsKeyPressed(KEY_R)) {
                camera.target = Vector2 { roundf(camera.target.x), roundf(camera.target.y) };
                camera.offset = Vector2 { roundf(screenWidth / 2.0f), roundf(screenHeight / 2.0f) };
                camera.rotation = 0.0f;
                camera.zoom = 1.0f;
                cameraReset = 0;
            }

            if (cameraFollowPlayer) {
                PlayerControllerState input{};
                QueryPlayerController(input);
                sim(now, dt, input, &charlie, &tilemap, slimes, coinSpriteDef);
                camera.target = body_ground_position(&charlie.body);
            } else {
                const int cameraSpeed = 5;
                if (IsKeyDown(KEY_A)) camera.target.x -= cameraSpeed / camera.zoom;
                if (IsKeyDown(KEY_D)) camera.target.x += cameraSpeed / camera.zoom;
                if (IsKeyDown(KEY_W)) camera.target.y -= cameraSpeed / camera.zoom;
                if (IsKeyDown(KEY_S)) camera.target.y += cameraSpeed / camera.zoom;
            }

            // Camera rotation controls
            //if (IsKeyPressed(KEY_Q)) {
            //    camera.rotation -= 45.0f;
            //    if (camera.rotation < 0.0f) camera.rotation += 360.0f;
            //}
            //else if (IsKeyPressed(KEY_E)) {
            //    camera.rotation += 45.0f;
            //    if (camera.rotation >= 360.0f) camera.rotation -= 360.0f;
            //}

            // Camera zoom controls
#if 0
            camera.zoom += GetMouseWheelMove() * 0.1f * camera.zoom;
#else
            const float mouseWheelMove = GetMouseWheelMove();
            if (mouseWheelMove) {
                //printf("zoom: %f, log: %f\n", camera.zoom, log10f(camera.zoom));
                camera.zoom *= mouseWheelMove > 0.0f ? 2.0f : 0.5f;
            }
#endif
        }

#if 1
        const int negZoomMultiplier = 7; // 7x negative zoom (out)
        const int posZoomMultiplier = 1; // 2x positive zoom (in)
        const float minZoom = 1.0f / (float)(1 << (negZoomMultiplier - 1));
        const float maxZoom = 1.0f * (float)(1 << (posZoomMultiplier - 1));
        camera.zoom = CLAMP(camera.zoom, minZoom, maxZoom);
        const float invZoom = 1.0f / camera.zoom;
#else
        const int negZoomMultiplier = 7; // 7x negative zoom (out)
        const int posZoomMultiplier = 2; // 2x positive zoom (in)
        const float minZoomPow2 = (float)(1 << (negZoomMultiplier - 1));
        const float maxZoomPow2 = (float)(1 << (posZoomMultiplier - 1));
        const float minZoom = 1.0f / minZoomPow2;
        const float maxZoom = 1.0f * maxZoomPow2;
        const float roundedZoom = roundf(camera.zoom * minZoomPow2) / minZoomPow2;
        camera.zoom = CLAMP(roundedZoom, minZoom, maxZoom);
        const float invZoom = 1.0f / camera.zoom;
#endif

        if (charlie.body.idle && !IsMusicPlaying(mus_whistle)) {
            PlayMusicStream(mus_whistle);
        } else if (!charlie.body.idle && IsMusicPlaying(mus_whistle)) {
            StopMusicStream(mus_whistle);
        }

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(ORANGE);
        DrawTexture(checkboardTexture, 0, 0, WHITE);

        Rectangle cameraRect = {};
        cameraRect.x = camera.target.x - camera.offset.x * invZoom;
        cameraRect.y = camera.target.y - camera.offset.y * invZoom;
        cameraRect.width  = camera.offset.x * 2.0f * invZoom;
        cameraRect.height = camera.offset.y * 2.0f * invZoom;

#if DEMO_VIEW_CULLING
        const float cameraRectPad = 100.0f * invZoom;
        cameraRect.x += cameraRectPad;
        cameraRect.y += cameraRectPad;
        cameraRect.width  -= cameraRectPad * 2.0f;
        cameraRect.height -= cameraRectPad * 2.0f;
#endif

        // TODO: Calculate this based on how many tiles will appear on the screen, rather than camera zoom
        // Alternatively, we could group nearby tiles of the same type together into large quads?
        const int zoomMipLevel = MAX(1, (int)invZoom / 8);

        BeginMode2D(camera);
        size_t tilesDrawn = 0;

        {
            const float tileWidthMip  = (float)(tilemap.tileset->tileWidth * zoomMipLevel);
            const float tileHeightMip = (float)(tilemap.tileset->tileHeight * zoomMipLevel);
            const size_t heightTiles = tilemap.heightTiles;
            const size_t widthTiles = tilemap.widthTiles;
            const Rectangle *tileRects = tilemap.tileset->textureRects;
            const float camLeft   = cameraRect.x;
            const float camTop    = cameraRect.y;
            const float camRight  = cameraRect.x + cameraRect.width;
            const float camBottom = cameraRect.y + cameraRect.height;

            for (size_t y = 0; y < heightTiles; y += zoomMipLevel) {
                for (size_t x = 0; x < widthTiles; x += zoomMipLevel) {
                    const Tile *tile = &tilemap.tiles[y * tilemap.widthTiles + x];
                    const Vector2 tilePos = tile->position;
                    if (tilePos.x + tileWidthMip >= camLeft &&
                        tilePos.y + tileHeightMip >= camTop &&
                        tilePos.x < camRight &&
                        tilePos.y < camBottom)
                    {
                        // Draw all tiles as textured rects (looks best, performs worst)
                        Rectangle textureRect = tileRects[tile->tileType];
                        DrawTextureRec(*tilemap.tileset->texture, textureRect, tile->position, WHITE);
                        tilesDrawn++;
                    }
                }
            }
        }
        //DrawRectangleRec(cameraRect, Fade(PINK, 0.5f));

        const Vector2 mousePosScreen = GetMousePosition();
        Vector2 mousePosWorld = {};
        mousePosWorld.x += mousePosScreen.x * invZoom + cameraRect.x;
        mousePosWorld.y += mousePosScreen.y * invZoom + cameraRect.y;

        Tile *mouseTile = 0;
        if (findMouseTile) {
            mouseTile = tilemap_at_world_try(&tilemap, (int)mousePosWorld.x, (int)mousePosWorld.y);
            if (mouseTile) {
                // Draw red outline on hovered tile
                Rectangle mouseTileRect {
                    mouseTile->position.x,
                    mouseTile->position.y,
                    (float)tilemap.tileset->tileWidth  * zoomMipLevel,
                    (float)tilemap.tileset->tileHeight * zoomMipLevel
                };
                DrawRectangleLinesEx(mouseTileRect, 1 * (int)invZoom, RED);
            }
        }

        {
            draw_commands_enable_culling(cameraRect);

            // Queue player for drawing
            player_push(&charlie);

            // Queue slimes for drawing
            for (int i = 0; i < MAX_SLIMES; i++) {
                const Slime *slime = &slimes[i];
                if (!slime->combat.hitPoints) {
                    continue;
                }

                slime_push(slime);
            }

            // Queue particles for drawing
            particles_push();

            draw_commands_flush();
        }

#if DEMO_VIEW_RSTAR
        AABB searchAABB = {
            mousePosWorld.x - 50,
            mousePosWorld.y - 50,
            mousePosWorld.x + 50,
            mousePosWorld.y + 50
        };
        Rectangle searchRect {
            searchAABB.min.x,
            searchAABB.min.y,
            searchAABB.max.x - searchAABB.min.x,
            searchAABB.max.y - searchAABB.min.y
        };

#if 1
        std::vector<int> matches{};
        tree.Search(searchAABB, matches);
#else
        static std::vector<int> matches;
        matches.clear();
        for (int i = 0; i < rects.size(); i++) {
            if (CheckCollisionRecs(rects[i], searchRect)) {
                matches.push_back(i);
            }
        }
#endif

        for (const int i : matches) {
            DrawRectangleRec(rects[i], RED);
            drawn[i] = true;
        }

        for (size_t i = 0; i < drawn.size(); i++) {
            if (!drawn[i]) {
                DrawRectangleLinesEx(rects[i], 2, WHITE);
            } else {
                drawn[i] = false;
            }
        }

        DrawRectangleLinesEx(searchRect, 2, YELLOW);
#endif

#if DEMO_VIEW_CULLING
        DrawRectangleLinesEx(cameraRect, 3, Fade(PINK, 0.8f));
#endif

#if DEMO_AI_TRACKING
        {
            Vector2 charlieCenter = player_get_bottom_center(&charlie);
            DrawCircle((int)charlieCenter.x, (int)charlieCenter.y, 640.0f, Fade(ORANGE, 0.5f));
        }
#endif

        EndMode2D();

        // Render mouse tile tooltip
        if (findMouseTile && mouseTile) {
            const float tooltipOffsetX = 10.0f;
            const float tooltipOffsetY = 10.0f;
            const float tooltipPad = 4.0f;

            float tooltipX = mousePosScreen.x + tooltipOffsetX;
            float tooltipY = mousePosScreen.y + tooltipOffsetY;
            const float tooltipW = 220.0f + tooltipPad * 2.0f;
            const float tooltipH = 40.0f  + tooltipPad * 2.0f;

            if (tooltipX + tooltipW > screenWidth ) tooltipX = screenWidth  - tooltipW;
            if (tooltipY + tooltipH > screenHeight) tooltipY = screenHeight - tooltipH;

            Rectangle tooltipRect{ tooltipX, tooltipY, tooltipW, tooltipH };
            DrawRectangleRec(tooltipRect, Fade(RAYWHITE, 0.8f));
            DrawRectangleLinesEx(tooltipRect, 1, Fade(BLACK, 0.8f));

            int lineOffset = 0;
            DrawTextFont(fonts[fontIdx], TextFormat("tilePos : %.02f, %.02f", mouseTile->position.x, mouseTile->position.y),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawTextFont(fonts[fontIdx], TextFormat("tileSize: %zu, %zu", tilemap.tileset->tileWidth, tilemap.tileset->tileHeight),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawTextFont(fonts[fontIdx], TextFormat("tileType: %d", mouseTile->tileType),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
        }

        // Render minimap
        const int minimapMargin = 6;
        const int minimapBorderWidth = 1;
        const int minimapX = screenWidth - minimapMargin - minimapTex.width - minimapBorderWidth * 2;
        const int minimapY = minimapMargin;
        const int minimapW = minimapTex.width + minimapBorderWidth * 2;
        const int minimapH = minimapTex.height + minimapBorderWidth * 2;
        const int minimapTexX = minimapX + minimapBorderWidth;
        const int minimapTexY = minimapY + minimapBorderWidth;
        DrawRectangleLines(minimapX, minimapY, minimapW, minimapH, BLACK);
        DrawTexture(minimapTex, minimapTexX, minimapTexY, WHITE);

        const char *text = 0;
        float hudCursorY = 0;

#define PUSH_TEXT(text, color) \
    DrawTextFont(fonts[fontIdx], text, margin + pad, hudCursorY, fontHeight, color); \
    hudCursorY += fontHeight + pad; \

        // Render HUD
        {
            int linesOfText = 8;
#if SHOW_DEBUG_STATS
            linesOfText += 7;
#endif
            const float margin = 6.0f;   // left/top margin
            const float pad = 4.0f;      // left/top pad
            const float hudWidth = 200.0f;
            const float hudHeight = linesOfText * (fontHeight + pad) + pad;

            hudCursorY += margin;

            const Color darkerGray{ 40, 40, 40, 255 };
            UNUSED(darkerGray);
            DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, DARKBLUE);
            DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

            hudCursorY += pad;

            if (gifRecording) {
                text = TextFormat("GIF RECORDING");
            } else {
                text = TextFormat("%2i fps (%.02f ms)", GetFPS(), GetFrameTime() * 1000.0f);
            }
            PUSH_TEXT(text, WHITE);
            text = TextFormat("Coins: %d", charlie.inventory.slots[PlayerInventorySlot_Coins].stackCount);
            PUSH_TEXT(text, YELLOW);

            text = TextFormat("Coins collected   %u", charlie.stats.coinsCollected);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Damage dealt      %.2f", charlie.stats.damageDealt);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Kilometers walked %.2f", charlie.stats.kmWalked);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Slimes slain      %u", charlie.stats.slimesSlain);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Times fist swung  %u", charlie.stats.timesFistSwung);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Times sword swung %u", charlie.stats.timesSwordSwung);
            PUSH_TEXT(text, LIGHTGRAY);

#if SHOW_DEBUG_STATS
            text = TextFormat("Zoom          %.03f", camera.zoom);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Zoom inverse  %.03f", invZoom);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Zoom mip      %zu", zoomMipLevel);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Tiles visible %zu", tilesDrawn);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Font index    %zu", fontIdx);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Particle FX   %zu", particle_effects_active());
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Particles     %zu", particles_active());
            PUSH_TEXT(text, GRAY);
#endif
        }

#if ALPHA_NETWORKING
        // Render connected clients
        if (args.server) {
            int linesOfText = 1 + (int)server.clientsConnected;

            const float margin = 6.0f;   // left/top margin
            const float pad = 4.0f;      // left/top pad
            const float hudWidth = 200.0f;
            const float hudHeight = linesOfText * (fontHeight + pad) + pad;

            hudCursorY += margin;

            const Color darkerGray{ 40, 40, 40, 255 };
            UNUSED(darkerGray);
            DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, DARKBLUE);
            DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

            hudCursorY += pad;

            PUSH_TEXT("Connected clients:", WHITE);

            for (size_t i = 0; i < NETWORK_SERVER_CLIENTS_MAX; i++) {
                const NetworkServerClient *serverClient = &server.clients[i];
                if (serverClient->address.host) {
                    text = TextFormatIP(serverClient->address);
                    PUSH_TEXT(text, WHITE);
                }
            }
        }

        // Render chat history
        {
            static GuiTextBoxAdvancedState chatInputState;

            if (chatActive) {
                const float margin = 6.0f;   // left/bottom margin
                const float pad = 4.0f;      // left/bottom pad

#if 0
                {
                    static bool dialog = true;
                    dialog = dialog || IsKeyPressed(KEY_GRAVE);

                    if (dialog) {
                        static float loginBoxW = 230.0f;
                        static float loginBoxH = 105.0f;

                        float mouseWheelMove = GetMouseWheelMove();
                        float *scrollFloat = IsKeyDown(KEY_LEFT_SHIFT) ? &loginBoxW : &loginBoxH;
                        if (mouseWheelMove > 0.0f) {
                            *scrollFloat += 25.0f;
                        } else if (mouseWheelMove < 0.0f) {
                            *scrollFloat = MAX(scrollFloat == &loginBoxH ? 104.0f : 223.0f, *scrollFloat - 25.0f);
                        }

                        Rectangle loginBoxRect = {
                            screenWidth/2.0f - loginBoxW/2.0f,
                            screenHeight/2.0f - loginBoxH/2.0f,
                            loginBoxW,
                            loginBoxH
                        };

                        static int loginBoxTextLen = 0;
                        static char loginBoxText[USERNAME_LENGTH_MAX + 1];
                        static GuiTextBoxAdvancedState loginBoxState;

                        int button = GuiTextInputBoxAdvanced(&loginBoxState, loginBoxRect, "Log In",
                            "Please enter your username:", "Login;Cancel", 1, loginBoxText, &loginBoxTextLen,
                            sizeof(loginBoxText), false);
                        switch (button) {
                            case 0: { // [X]
                                dialog = false;
                                break;
                            }
                            case 1: { // Login
                                if (loginBoxTextLen) {
                                    network_client_send_chat_message(&client, loginBoxText, loginBoxTextLen);
                                }
                                break;
                            }
                            case 2: { // Cancel
                                dialog = false;
                                break;
                            }
                        }

                        if (escape || button >= 0) {
                            memset(loginBoxText, 0, sizeof(loginBoxText));
                            loginBoxTextLen = 0;
                        }
                    }
                }
#endif

                const int linesOfText = (int)client.chatHistory.count;
                const float chatWidth = 420.0f;
                const float chatHeight = linesOfText * (fontHeight + pad) + pad;
                const float inputBoxHeight = fontHeight + pad * 2.0f;
                const float chatX = margin;
                const float chatY = screenHeight - margin - inputBoxHeight - chatHeight;

                // NOTE: The chat history renders from the bottom up (most recent message first)
                float cursorY = (chatY + chatHeight) - pad - fontHeight;
                const char *text = 0;

                DrawRectangle((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(DARKGRAY, 0.8f));
                DrawRectangleLines((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(BLACK, 0.8f));

                for (int i = (int)client.chatHistory.count - 1; i >= 0; i--) {
                    size_t messageIdx = (client.chatHistory.first + i) % client.chatHistory.capacity;
                    const ChatMessage *message = &client.chatHistory.messages[messageIdx];
                    assert(message->messageLength);

                    text = TextFormat("[%s][%.*s]: %.*s", "00:00:00", message->usernameLength, message->username,
                        message->messageLength, message->message);
                    DrawTextFont(fonts[fontIdx], text, margin + pad, cursorY, fontHeight, WHITE);
                    cursorY -= fontHeight + pad;
                }

                static int chatInputTextLen = 0;
                static char chatInputText[CHAT_MESSAGE_BUFFER_LEN];
                assert(CHAT_MESSAGE_LENGTH_MAX < CHAT_MESSAGE_BUFFER_LEN);

                Rectangle chatInputRect = { margin, screenHeight - margin - inputBoxHeight, chatWidth, inputBoxHeight };
                //GuiTextBox(inputBox, chatInputText, CHAT_MESSAGE_LENGTH_MAX, true);
                //GuiTextBoxEx(inputBox, chatInputText, CHAT_MESSAGE_LENGTH_MAX, true);
                if (GuiTextBoxAdvanced(&chatInputState, chatInputRect, chatInputText, &chatInputTextLen, CHAT_MESSAGE_LENGTH_MAX, false)) {
                    size_t messageLength = chatInputTextLen;
                    if (messageLength) {
                        network_client_send_chat_message(&client, chatInputText, messageLength);
                        memset(chatInputText, 0, sizeof(chatInputText));
                        chatInputTextLen = 0;
                    }
                }

                if (escape) {
                    chatActive = false;
                    escape = false;
                    memset(chatInputText, 0, sizeof(chatInputText));
                    chatInputTextLen = 0;
                }
            }

            // HACK: Check for this *after* rendering chat to avoid pressing "t" causing it to show up in the chat box
            if (!chatActive && IsKeyDown(KEY_T)) {
                chatActive = true;
                GuiSetActiveTextbox(&chatInputState);
            }
        }
#endif
#undef PUSH_TEXT

        EndDrawing();
        //----------------------------------------------------------------------------------

        // If nobody else handled the escape key, time to exit!
        if (escape) {
            break;
        }
    }

    //--------------------------------------------------------------------------------------
    // Clean up
    //--------------------------------------------------------------------------------------
#if ALPHA_NETWORKING
    network_client_close_socket(&client);
    network_client_free(&client);
    if (args.server) {
        network_server_close_socket(&server);
        server_thread.join();
    }
    zed_net_shutdown();
#endif
    tilemap_free(&tilemap);
    tileset_free(&tileset);
    UnloadTexture(minimapTex);
    UnloadTexture(tilesetTex);
    UnloadTexture(checkboardTexture);
    sound_catalog_free();
    spritesheet_catalog_free();
    particles_free();
    draw_commands_free();
    UnloadMusicStream(mus_whistle);
    UnloadMusicStream(mus_background);
    CloseAudioDevice();
    CloseWindow();
    for (size_t i = 0 ; i < ARRAY_SIZE(fonts); i++) {
        UnloadFont(fonts[i]);
    }

    fclose(logFile);
    return 0;
}