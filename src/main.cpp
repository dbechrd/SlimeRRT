#include "args.h"
#include "bit_stream.h"
#include "chat.h"
#include "controller.h"
#include "draw_command.h"
#include "error.h"
#include "game_server.h"
#include "healthbar.h"
#include "helpers.h"
#include "item_catalog.h"
#include "maths.h"
#include "net_client.h"
#include "net_server.h"
#include "particles.h"
#include "particle_fx.h"
#include "player.h"
#include "slime.h"
#include "sound_catalog.h"
#include "spritesheet.h"
#include "spritesheet_catalog.h"
#include "tileset.h"
#include "tilemap.h"
#include "rtree.h"
#include "world.h"
#include "../test/tests.h"

#include "dlb_rand.h"
#include "raylib.h"
#include "raygui.h"
#include "gui_textbox_extended.h"
#include "zed_net.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <array>
#include <thread>

int main(int argc, char *argv[])
{
#if _DEBUG
    run_tests();
#endif

    Args args{ argc, argv };
    //args.server = true;

    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    error_init();

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

    std::thread game_server_thread;
    if (args.server) {
        game_server_thread = std::thread(game_server_run, args);
    }

    NetClient client{};
    strncpy(client.username, args.server ? "SERVER" : "client", ARRAY_SIZE(client.username));
    client.usernameLength = strnlen(client.username, ARRAY_SIZE(client.username));
    if (!net_client_init(&client)) {
        TraceLog(LOG_FATAL, "Failed to initialize network client system.\n");
    }

    if (!net_client_open_socket(&client)) {
        TraceLog(LOG_ERROR, "Failed to open client socket.\n");
        // TODO: Handle "offline mode" gracefully. This shouldn't prevent people people from playing single player
        exit(-1);
    }

    //if (!net_client_connect(&client, "slime.theprogrammingjunkie.com", 4040)) {
    if (!net_client_connect(&client, "127.0.0.1", 4040)) {
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

    HealthBar::SetFont(fonts[0]);

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

    DrawList drawList{};
    sound_catalog_init();
    g_spritesheetCatalog.Load();
    particles_init();

    Music mus_background{};
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

    Tileset tileset{ &tilesetTex, 32, 32, (int)TileType::Count };
    World world{};

    Texture minimapTex = {};
    {
        Image minimapImg = {};
        // NOTE: This is the client-side world map. Fog of war until tile types known?
        minimapImg.width = (int)world.map.width;
        minimapImg.height = (int)world.map.height;
        minimapImg.mipmaps = 1;
        minimapImg.format = UNCOMPRESSED_R8G8B8A8;
        assert(sizeof(Color) == 4);
        minimapImg.data = calloc((size_t)minimapImg.width * minimapImg.height, sizeof(Color));
        assert(minimapImg.data);

        Color tileColors[(int)TileType::Count]{};
        tileColors[(int)TileType::Grass   ] = GREEN;
        tileColors[(int)TileType::Water   ] = SKYBLUE;
        tileColors[(int)TileType::Forest  ] = DARKGREEN;
        tileColors[(int)TileType::Wood    ] = BROWN;
        tileColors[(int)TileType::Concrete] = GRAY;

        const size_t height = world.map.height;
        const size_t width = world.map.width;

        Color *minimapPixel = (Color *)minimapImg.data;
        for (size_t y = 0; y < height; y += 1) {
            for (size_t x = 0; x < width; x += 1) {
                const Tile *tile = &world.map.tiles[y * world.map.width + x];
                // Draw all tiles as different colored pixels
                assert((int)tile->tileType >= 0);
                assert((int)tile->tileType < (int)TileType::Count);
                *minimapPixel = tileColors[(int)tile->tileType];
                minimapPixel++;
            }
        }

        minimapTex = LoadTextureFromImage(minimapImg);
        free(minimapImg.data);
    }

    const Vector3 worldSpawn = {
        (float)world.map.width / 2.0f * world.map.tileWidth,
        (float)world.map.height / 2.0f * world.map.tileHeight,
        0.0f
    };

#if DEMO_VIEW_RTREE
    const int RECT_COUNT = 100;
    std::array<Rectangle, RECT_COUNT> rects{};
    std::array<bool, RECT_COUNT> drawn{};
    RTree::RTree<size_t> tree{};

    dlb_rand32_t rstar_rand{};
    dlb_rand32_seed_r(&rstar_rand, 0, 0);
    size_t next_rect_to_add = 0; //RECT_COUNT;
    double lastRectAddedAt = GetTime();
    for (size_t i = 0; i < rects.size(); i++) {
        rects[i].x = worldSpawn.x + dlb_rand32f_variance_r(&rstar_rand, 600.0f);
        rects[i].y = worldSpawn.y + dlb_rand32f_variance_r(&rstar_rand, 300.0f);
        rects[i].width  = dlb_rand32f_range_r(&rstar_rand, 50.0f, 100.0f);
        rects[i].height = dlb_rand32f_range_r(&rstar_rand, 50.0f, 100.0f);
        if (i < next_rect_to_add) {
            AABB aabb{};
            aabb.min.x = rects[i].x;
            aabb.min.y = rects[i].y;
            aabb.max.x = rects[i].x + rects[i].width;
            aabb.max.y = rects[i].y + rects[i].height;
            tree.Insert(aabb, i);
        }
    }
#endif

    Camera2D camera = {};
    //camera.target = (Vector2){
    //    (float)tilemap.width / 2.0f * tilemap.tileset->tileWidth,
    //    (float)tilemap.height / 2.0f * tilemap.tileset->tileHeight
    //};
    camera.offset.x = screenWidth / 2.0f;
    camera.offset.y = screenHeight / 2.0f;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    bool cameraReset = false;
    bool cameraFollowPlayer = true;

    // TODO: Move sprite loading to somewhere more sane
    const Spritesheet &charlieSpritesheet = g_spritesheetCatalog.spritesheets[(int)SpritesheetID::Charlie];
    const SpriteDef *charlieSpriteDef = charlieSpritesheet.FindSprite("player_sword");
    assert(charlieSpriteDef);

    const Spritesheet &slimeSpritesheet = g_spritesheetCatalog.spritesheets[(int)SpritesheetID::Slime];
    const SpriteDef *slimeSpriteDef = slimeSpritesheet.FindSprite("slime");
    assert(slimeSpriteDef);

    const Spritesheet &coinSpritesheet = g_spritesheetCatalog.spritesheets[(int)SpritesheetID::Coin];
    const SpriteDef *coinSpriteDef = coinSpritesheet.FindSprite("coin");
    assert(coinSpriteDef);

    Player charlie("Charlie", *charlieSpriteDef);
    charlie.body.position = worldSpawn;
    world.player = &charlie;

    {
        const float slimeRadius = 50.0f;
        const size_t mapPixelsX = world.map.width  * world.map.tileWidth;
        const size_t mapPixelsY = world.map.height * world.map.tileHeight;
        const float maxX = mapPixelsX - slimeRadius;
        const float maxY = mapPixelsY - slimeRadius;
        for (size_t i = 0; i < 256; i++) {
            //world.slimes.emplace_back(nullptr, *slimeSpriteDef);
            world.slimes.emplace_back(Slime{ nullptr, *slimeSpriteDef });
            Slime &slime = world.slimes.back();
            slime.body.position.x = dlb_rand32f_range(slimeRadius, maxX);
            slime.body.position.y = dlb_rand32f_range(slimeRadius, maxY);
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
        net_client_receive(&client);
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
                net_client_send_chat_message(&client, CSTR("User pressed the C key."));
            }
#endif

            if (IsKeyPressed(KEY_F)) {
                cameraFollowPlayer = !cameraFollowPlayer;
            }

            if (IsKeyDown(KEY_N)) {
                if (next_rect_to_add < RECT_COUNT && (GetTime() - lastRectAddedAt > 0.1)) {
                    AABB aabb{};
                    aabb.min.x = floorf(rects[next_rect_to_add].x);
                    aabb.min.y = floorf(rects[next_rect_to_add].y);
                    aabb.max.x = floorf(rects[next_rect_to_add].x + rects[next_rect_to_add].width);
                    aabb.max.y = floorf(rects[next_rect_to_add].y + rects[next_rect_to_add].height);
                    tree.Insert(aabb, next_rect_to_add);
                    next_rect_to_add++;
                    lastRectAddedAt = GetTime();
                }
            } else {
                lastRectAddedAt = 0;
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
                PlayerControllerState input = PlayerControllerState::Query();
                world.Sim(now, dt, input, coinSpriteDef);
                camera.target = charlie.body.GroundPosition();
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
            const float tileWidthMip  = (float)(world.map.tileWidth * zoomMipLevel);
            const float tileHeightMip = (float)(world.map.tileHeight * zoomMipLevel);
            const size_t height = world.map.height;
            const size_t width  = world.map.width;
            const Rectangle *tileRects = tileset.textureRects;
            const float camLeft   = cameraRect.x;
            const float camTop    = cameraRect.y;
            const float camRight  = cameraRect.x + cameraRect.width;
            const float camBottom = cameraRect.y + cameraRect.height;

            for (size_t y = 0; y < height; y += zoomMipLevel) {
                for (size_t x = 0; x < width; x += zoomMipLevel) {
                    const Tile *tile = &world.map.tiles[y * world.map.width + x];
                    const Vector2 tilePos = tile->position;
                    if (tilePos.x + tileWidthMip >= camLeft &&
                        tilePos.y + tileHeightMip >= camTop &&
                        tilePos.x < camRight &&
                        tilePos.y < camBottom)
                    {
                        // Draw all tiles as textured rects (looks best, performs worst)
                        Rectangle textureRect = tileRects[(int)tile->tileType];
                        DrawTextureRec(*tileset.texture, textureRect, tile->position, WHITE);
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
            mouseTile = tilemap_at_world_try(&world.map, (int)mousePosWorld.x, (int)mousePosWorld.y);
            if (mouseTile) {
                // Draw red outline on hovered tile
                Rectangle mouseTileRect {
                    mouseTile->position.x,
                    mouseTile->position.y,
                    (float)world.map.tileWidth  * zoomMipLevel,
                    (float)world.map.tileHeight * zoomMipLevel
                };
                DrawRectangleLinesEx(mouseTileRect, 1 * (int)invZoom, RED);
            }
        }

        {
            drawList.EnableCulling(cameraRect);

            // Queue player for drawing
            drawList.Push(charlie);

            // Queue slimes for drawing
            for (const Slime &slime : world.slimes) {
                if (slime.combat.hitPoints) {
                    drawList.Push(slime);
                }
            }

            // Queue particles for drawing
            particles_push(drawList);

            drawList.Flush();
        }

#if DEMO_VIEW_RTREE
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
        std::vector<size_t> matches{};
        tree.Search(searchAABB, matches, RTree::CompareMode::Overlap);
#else
        static std::vector<void *> matches;
        matches.clear();
        for (size_t i = 0; i < rects.size(); i++) {
            if (CheckCollisionRecs(rects[i], searchRect)) {
                matches.push_back((void *)i);
            }
        }
#endif

        for (const size_t i : matches) {
            DrawRectangleRec(rects[i], Fade(RED, 0.6f));
            drawn[i] = true;
        }

        //for (size_t i = 0; i < drawn.size(); i++) {
        //    if (!drawn[i]) {
        //        DrawRectangleLinesEx(rects[i], 2, WHITE);
        //    } else {
        //        drawn[i] = false;
        //    }
        //}

        tree.Draw();
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
            DrawTextFont(fonts[fontIdx], TextFormat("tileSize: %zu, %zu", world.map.tileWidth, world.map.tileHeight),
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
            text = TextFormat("Coins: %d", charlie.inventory.slots[(int)PlayerInventorySlot::Coins].stackCount);
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
            int linesOfText = 1 + (int)g_net_server.clientsConnected;

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

            for (size_t i = 0; i < NET_SERVER_CLIENTS_MAX; i++) {
                const NetworkServerClient *serverClient = &g_net_server.clients[i];
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

#if 1
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
                                    client.usernameLength = MIN(ARRAY_SIZE(client.username), loginBoxTextLen);
                                    strncpy(client.username, loginBoxText, client.usernameLength);
                                    net_client_send_chat_message(&client, CSTR("Auth!"));
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
                const char *chatText = 0;
                Color chatColor = WHITE;

                DrawRectangle((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(DARKGRAY, 0.8f));
                DrawRectangleLines((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(BLACK, 0.8f));

                for (int i = (int)client.chatHistory.count - 1; i >= 0; i--) {
                    size_t messageIdx = (client.chatHistory.first + i) % client.chatHistory.capacity;
                    const ChatMessage *message = &client.chatHistory.messages[messageIdx];
                    assert(message->messageLength);

                    if (message->usernameLength == 6 && !strncmp(message->username, "SERVER", message->usernameLength)) {
                        chatText = TextFormat("[%s]<SERVER>: %.*s", "00:00:00", message->messageLength, message->message);
                        chatColor = RED;
                    } else {
                        chatText = TextFormat("[%s][%.*s]: %.*s", "00:00:00", message->usernameLength, message->username,
                            message->messageLength, message->message);
                        chatColor = WHITE;
                    }
                    DrawTextFont(fonts[fontIdx], chatText, margin + pad, cursorY, fontHeight, chatColor);
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
                        net_client_send_chat_message(&client, chatInputText, messageLength);
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
    net_client_close_socket(&client);
    net_client_free(&client);
    if (args.server) {
        net_server_close_socket(&g_net_server);
        game_server_thread.join();
    }
    zed_net_shutdown();
#endif
    UnloadTexture(minimapTex);
    UnloadTexture(tilesetTex);
    UnloadTexture(checkboardTexture);
    sound_catalog_free();
    particles_free();
    UnloadMusicStream(mus_whistle);
    UnloadMusicStream(mus_background);
    CloseAudioDevice();
    CloseWindow();
    for (size_t i = 0 ; i < ARRAY_SIZE(fonts); i++) {
        UnloadFont(fonts[i]);
    }

    return 0;
}