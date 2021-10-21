#include "draw_command.h"
#include "game_client.h"
#include "healthbar.h"
#include "item_catalog.h"
#include "loot_table.h"
#include "net_client.h"
#include "particles.h"
#include "rtree.h"
#include "sound_catalog.h"
#include "spritesheet_catalog.h"
#include "dlb_types.h"

#include "raylib/raygui.h"
#include "raylib/raylib.h"
#define GRAPHICS_API_OPENGL_33
#include "raylib/rlgl.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"
#include "GLFW/glfw3.h"

#include <chrono>
#include <future>

using namespace std::chrono_literals;

const char *GameClient::LOG_SRC = "GameClient";

ErrorType GameClient::Run(const char *hostname, unsigned short port)
{
    World lobby{};
    Texture minimapTex{};
    Texture tilesetTex{};
    Texture checkboardTexture{};
    Music mus_background{};
    Music mus_whistle{};
    Font fonts[3]{};

E_START
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    GLFWwindow *glfwWindow = glfwGetCurrentContext();
    assert(glfwWindow);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    strncpy(netClient.username, "CLIENT", ARRAY_SIZE(netClient.username));
    netClient.usernameLength = strnlen(netClient.username, ARRAY_SIZE(netClient.username));

    E_CHECK(netClient.OpenSocket(), "Failed to open client socket");
    E_CHECK(netClient.Connect(hostname, port), "Failed to connect client");

    const int fontHeight = 14;
    fonts[0] = GetFontDefault();
    fonts[1] = LoadFontEx("C:/Windows/Fonts/consola.ttf", fontHeight, 0, 0);
    fonts[2] = LoadFontEx("resources/UbuntuMono-Regular.ttf", fontHeight, 0, 0);
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
    //SetMasterVolume(0.2f); // 0.01f);

    DrawList drawList{};
    sound_catalog_init();
    SpritesheetCatalog spritesheetCatalog{};
    spritesheetCatalog.Load();
    particles_init();
    ItemCatalog::instance.Load();
    loot_table_init();

    float mus_background_vmin = 0.02f;
    float mus_background_vmax = 0.2f;
    mus_background = LoadMusicStream("resources/fluquor_copyright.ogg");
    mus_background.looping = true;
    //PlayMusicStream(mus_background);
    SetMusicVolume(mus_background, mus_background_vmax);
    UpdateMusicStream(mus_background);

    float mus_whistle_vmin = 0.0f;
    float mus_whistle_vmax = 1.0f;
    mus_whistle = LoadMusicStream("resources/whistle.ogg");
    SetMusicVolume(mus_whistle, mus_whistle_vmin);
    mus_whistle.looping = true;

    float whistleAlpha = 0.0f;

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    tilesetTex = LoadTexture("resources/tiles32.png");
    assert(tilesetTex.width);

    Tileset tileset{ &tilesetTex, 32, 32, (int)TileType::Count };

    tilemap_generate_lobby(&lobby.map);
    world = &lobby;

    {
        Image minimapImg{};
        // NOTE: This is the client-side world map. Fog of war until tile types known?
        minimapImg.width = (int)world->map.width;
        minimapImg.height = (int)world->map.height;
        minimapImg.mipmaps = 1;
        minimapImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        assert(sizeof(Color) == 4);
        minimapImg.data = calloc((size_t)minimapImg.width * minimapImg.height, sizeof(Color));
        assert(minimapImg.data);

        Color tileColors[(int)TileType::Count]{};
        tileColors[(int)TileType::Grass   ] = GREEN;
        tileColors[(int)TileType::Water   ] = SKYBLUE;
        tileColors[(int)TileType::Forest  ] = DARKGREEN;
        tileColors[(int)TileType::Wood    ] = BROWN;
        tileColors[(int)TileType::Concrete] = GRAY;

        const size_t height = world->map.height;
        const size_t width = world->map.width;

        Color *minimapPixel = (Color *)minimapImg.data;
        for (size_t y = 0; y < height; y += 1) {
            for (size_t x = 0; x < width; x += 1) {
                const Tile *tile = &world->map.tiles[y * world->map.width + x];
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
        (float)world->map.width / 2.0f * world->map.tileWidth,
        (float)world->map.height / 2.0f * world->map.tileHeight,
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
        rects[i].width = dlb_rand32f_range_r(&rstar_rand, 50.0f, 100.0f);
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

    Camera2D camera{};
    //camera.target = (Vector2){
    //    (float)tilemap.width / 2.0f * tilemap.tileset->tileWidth,
    //    (float)tilemap.height / 2.0f * tilemap.tileset->tileHeight
    //};
    camera.offset.x = screenWidth / 2.0f;
    camera.offset.y = screenHeight / 2.0f;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    bool cameraReset = false;
    bool cameraFree = false;

    // TODO: Move sprite loading to somewhere more sane
    const Spritesheet &charlieSpritesheet = spritesheetCatalog.spritesheets[(int)SpritesheetID::Charlie];
    const SpriteDef *charlieSpriteDef = charlieSpritesheet.FindSprite("player_sword");
    assert(charlieSpriteDef);

    const Spritesheet &slimeSpritesheet = spritesheetCatalog.spritesheets[(int)SpritesheetID::Slime];
    const SpriteDef *slimeSpriteDef = slimeSpritesheet.FindSprite("slime");
    assert(slimeSpriteDef);

    const Spritesheet &coinSpritesheet = spritesheetCatalog.spritesheets[(int)SpritesheetID::Coin];
    const SpriteDef *coinSpriteDef = coinSpritesheet.FindSprite("coin");
    assert(coinSpriteDef);

    Player charlie("Charlie", *charlieSpriteDef);
    charlie.body.position = worldSpawn;
    world->player = &charlie;

    {
        const float slimeRadius = 50.0f;
        const size_t mapPixelsX = world->map.width * world->map.tileWidth;
        const size_t mapPixelsY = world->map.height * world->map.tileHeight;
        const float maxX = mapPixelsX - slimeRadius;
        const float maxY = mapPixelsY - slimeRadius;

        world->slimes.emplace_back("Sam", slimeSpriteDef);
        //world->slimes.emplace_back(Slime{ nullptr, *slimeSpriteDef });
        Slime &sam = world->slimes.back();
        sam.combat.maxHitPoints = 100000.0f;
        sam.combat.hitPoints = sam.combat.maxHitPoints;
        sam.combat.meleeDamage = 0.0f;
        sam.combat.lootTableId = LootTableID::LT_Sam;
        sam.body.position.x = dlb_rand32f_range(slimeRadius, maxX);
        sam.body.position.y = dlb_rand32f_range(slimeRadius, maxY);
        sam.body.position = v3_add(worldSpawn, { 0, -300.0f, 0 });
        sam.sprite.scale = 2.0f;
    }

    double frameStart = GetTime();
    double dt = 0;
    const int targetFPS = 60;
    //SetTargetFPS(targetFPS);
    SetWindowState(FLAG_VSYNC_HINT);
    bool gifRecording = false;
    bool chatActive = false;
    //---------------------------------------------------------------------------------------

#define KEY_DOWN(key)

    // Main game loop
    while (!WindowShouldClose()) {
        // NOTE: Limit delta time to 2 frames worth of updates to prevent chaos for large dt (e.g. when debugging)
        const double dtMax = (1.0 / targetFPS) * 2;
        const double now = GetTime();
        dt = MIN(now - frameStart, dtMax);
        frameStart = now;

        const bool imguiUsingMouse = io.WantCaptureMouse;
        const bool imguiUsingKeyboard = io.WantCaptureKeyboard;
        PlayerControllerState input = PlayerControllerState::Query(!io.WantCaptureMouse, !io.WantCaptureKeyboard, cameraFree);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static bool show_demo_window = false;
        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        bool escape = input.escape;
        bool findMouseTile = false;

        // HACK: No way to check if Raylib is currently recording.. :(
        if (gifRecording) {
            dt = 1.0 / 10.0;
        }

        E_CHECK(netClient.Receive(), "Failed to receive packets");

        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            cameraReset = 1;
        }

        UpdateMusicStream(mus_background);
        UpdateMusicStream(mus_whistle);

        if (!chatActive) {
            findMouseTile = input.dbg_findMouseTile;

            if (input.screenshot) {
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                char screenshotName[64]{};
                int len = snprintf(screenshotName, sizeof(screenshotName),
                    "screenshots/%d-%02d-%02d_%02d-%02d-%02d_screenshot.png",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                assert(len < sizeof(screenshotName));
                TakeScreenshot(screenshotName);
            }

            if (input.dbg_imgui) {
                show_demo_window = !show_demo_window;
            }

            if (input.dbg_nextFont) {
                fontIdx++;
                if (fontIdx >= ARRAY_SIZE(fonts)) {
                    fontIdx = 0;
                    GuiSetFont(fonts[fontIdx]);
                }
            }

            if (IsKeyPressed(KEY_F10)) {
                ToggleFullscreen();
            }

            if (input.dbg_toggleVsync) {
                IsWindowState(FLAG_VSYNC_HINT) ? ClearWindowState(FLAG_VSYNC_HINT) : SetWindowState(FLAG_VSYNC_HINT);
            }

            if (input.dbg_chatMessage) {
                netClient.SendChatMessage(CSTR("User pressed the C key."));
            }

            if (input.dbg_toggleFreecam) {
                cameraFree = !cameraFree;
            }
#if DEMO_VIEW_RTREE
            if (input.dbg_nextRtreeRect) {
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
#endif
            const float cameraSpeedDefault = 5.0f;
            static float cameraSpeed = cameraSpeedDefault;

            // Camera reset (zoom and rotation)
            if (cameraReset || input.cameraReset) {
                camera.offset = Vector2{ roundf(screenWidth / 2.0f), roundf(screenHeight / 2.0f) };
                camera.target = Vector2{ roundf(camera.target.x), roundf(camera.target.y) };
                camera.rotation = 0.0f;
                camera.zoom = 1.0f;
                cameraSpeed = cameraSpeedDefault;
                cameraReset = 0;
            }

            if (!cameraFree) {
                world->Sim(now, dt, input, coinSpriteDef);
                camera.target = charlie.body.GroundPosition();
                camera.rotation = 0.0f;
                camera.zoom = 1.0f;
                cameraSpeed = cameraSpeedDefault;
            } else {
                cameraSpeed += input.cameraSpeedDelta;
                if (input.cameraWest)  camera.target.x -= cameraSpeed / camera.zoom;
                if (input.cameraEast)  camera.target.x += cameraSpeed / camera.zoom;
                if (input.cameraNorth) camera.target.y -= cameraSpeed / camera.zoom;
                if (input.cameraSouth) camera.target.y += cameraSpeed / camera.zoom;
                if (input.cameraRotateCW) {
                    camera.rotation += 45.0f;
                    if (camera.rotation >= 360.0f) camera.rotation -= 360.0f;
                } else if (input.cameraRotateCCW) {
                    camera.rotation -= 45.0f;
                    if (camera.rotation < 0.0f) camera.rotation += 360.0f;
                }
                // Camera zoom controls
#if 0
                camera.zoom += input.cameraZoomDelta * 0.1f * camera.zoom;
#elif 0
                if (input.cameraZoomDelta) {
                    //printf("zoom: %f, log: %f\n", camera.zoom, log10f(camera.zoom));
                    camera.zoom *= input.cameraZoomDelta > 0.0f ? 2.0f : 0.5f;
                }
#else
                static bool zoomOut = false;
                if (input.cameraZoomOut) zoomOut = !zoomOut;
                camera.zoom = zoomOut ? 0.5f : 1.0f;
#endif
            }
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

        if (charlie.body.idle && whistleAlpha < 1.0f) {
            whistleAlpha = CLAMP(whistleAlpha + 0.005f, 0.0f, 1.0f);
            SetMusicVolume(mus_background, LERP(mus_background_vmin, mus_background_vmax, 1.0f - whistleAlpha));
            SetMusicVolume(mus_whistle, LERP(mus_whistle_vmin, mus_whistle_vmax, whistleAlpha));
            if (!IsMusicPlaying(mus_whistle)) {
                PlayMusicStream(mus_whistle);
            }
        } else if (!charlie.body.idle && whistleAlpha > 0.0f) {
            whistleAlpha = CLAMP(whistleAlpha - 0.01f, 0.0f, 1.0f);
            SetMusicVolume(mus_background, LERP(mus_background_vmin, mus_background_vmax, 1.0f - whistleAlpha));
            SetMusicVolume(mus_whistle, LERP(mus_whistle_vmin, mus_whistle_vmax, whistleAlpha));
            if (whistleAlpha == 0.0f) {
                StopMusicStream(mus_whistle);
            }
        }

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(ORANGE);
        DrawTexture(checkboardTexture, 0, 0, WHITE);

        Rectangle cameraRect{};
        cameraRect.x = camera.target.x - camera.offset.x * invZoom;
        cameraRect.y = camera.target.y - camera.offset.y * invZoom;
        cameraRect.width = camera.offset.x * 2.0f * invZoom;
        cameraRect.height = camera.offset.y * 2.0f * invZoom;

#if DEMO_VIEW_CULLING
        const float cameraRectPad = 100.0f * invZoom;
        cameraRect.x += cameraRectPad;
        cameraRect.y += cameraRectPad;
        cameraRect.width -= cameraRectPad * 2.0f;
        cameraRect.height -= cameraRectPad * 2.0f;
#endif

        // TODO: Calculate this based on how many tiles will appear on the screen, rather than camera zoom
        // Alternatively, we could group nearby tiles of the same type together into large quads?
        const int zoomMipLevel = MAX(1, (int)invZoom / 8);

        BeginMode2D(camera);
        size_t tilesDrawn = 0;

        {
            const float tileWidthMip = (float)(world->map.tileWidth * zoomMipLevel);
            const float tileHeightMip = (float)(world->map.tileHeight * zoomMipLevel);
            const size_t height = world->map.height;
            const size_t width = world->map.width;
            const Rectangle *tileRects = tileset.textureRects;
            const float camLeft = cameraRect.x;
            const float camTop = cameraRect.y;
            const float camRight = cameraRect.x + cameraRect.width;
            const float camBottom = cameraRect.y + cameraRect.height;

            for (size_t y = 0; y < height; y += zoomMipLevel) {
                for (size_t x = 0; x < width; x += zoomMipLevel) {
                    const Tile *tile = &world->map.tiles[y * world->map.width + x];
                    const Vector2 tilePos = tile->position;
                    if (tilePos.x + tileWidthMip >= camLeft &&
                        tilePos.y + tileHeightMip >= camTop &&
                        tilePos.x < camRight &&
                        tilePos.y < camBottom)                     {
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
        Vector2 mousePosWorld{};
        mousePosWorld.x += mousePosScreen.x * invZoom + cameraRect.x;
        mousePosWorld.y += mousePosScreen.y * invZoom + cameraRect.y;

        Tile *mouseTile = 0;
        if (findMouseTile) {
            mouseTile = tilemap_at_world_try(&world->map, (int)mousePosWorld.x, (int)mousePosWorld.y);
            if (mouseTile) {
                // Draw red outline on hovered tile
                Rectangle mouseTileRect{
                    mouseTile->position.x,
                    mouseTile->position.y,
                    (float)world->map.tileWidth * zoomMipLevel,
                    (float)world->map.tileHeight * zoomMipLevel
                };
                DrawRectangleLinesEx(mouseTileRect, 1 * (int)invZoom, RED);
            }
        }

        {
            drawList.EnableCulling(cameraRect);

            // Queue player for drawing
            drawList.Push(charlie);

            // Queue slimes for drawing
            for (const Slime &slime : world->slimes) {
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
        Rectangle searchRect{
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
            Vector3 charlieCenter = sprite_world_center(charlie.sprite, charlie.body.position, charlie.sprite.scale);
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
            const float tooltipH = 40.0f + tooltipPad * 2.0f;

            if (tooltipX + tooltipW > screenWidth) tooltipX = screenWidth - tooltipW;
            if (tooltipY + tooltipH > screenHeight) tooltipY = screenHeight - tooltipH;

            Rectangle tooltipRect{ tooltipX, tooltipY, tooltipW, tooltipH };
            DrawRectangleRec(tooltipRect, Fade(RAYWHITE, 0.8f));
            DrawRectangleLinesEx(tooltipRect, 1, Fade(BLACK, 0.8f));

            int lineOffset = 0;
            DrawTextFont(fonts[fontIdx], TextFormat("tilePos : %.02f, %.02f", mouseTile->position.x, mouseTile->position.y),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawTextFont(fonts[fontIdx], TextFormat("tileSize: %zu, %zu", world->map.tileWidth, world->map.tileHeight),
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

#if 0
        // TODO: Keep track of list of other clients in GameClient
        // TODO: Keep track of who is the host in NetServer (is host==127.0.0.1 sufficient?)
        // Render connected clients
        if (args.server) {
            int linesOfText = 1 + (int)gameServer.clients.size();

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

            for (auto &kv : gameServer.clients) {
                text = TextFormatIP(kv.second.address);
                PUSH_TEXT(text, WHITE);
            }
        }
#endif

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
                            screenWidth / 2.0f - loginBoxW / 2.0f,
                            screenHeight / 2.0f - loginBoxH / 2.0f,
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
                        case 0:
                        { // [X]
                            dialog = false;
                            break;
                        }
                        case 1:
                        { // Login
                            if (loginBoxTextLen) {
                                netClient.usernameLength = MIN(ARRAY_SIZE(netClient.username), loginBoxTextLen);
                                strncpy(netClient.username, loginBoxText, netClient.usernameLength);
                                netClient.SendChatMessage(CSTR("Auth!"));
                            }
                            break;
                        }
                        case 2:
                        { // Cancel
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

                const int linesOfText = (int)netClient.chatHistory.Count();
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

                for (int i = (int)netClient.chatHistory.Count() - 1; i >= 0; i--) {
                    const ChatMessage &chatMsg = netClient.chatHistory.At(i);
                    assert(chatMsg.messageLength);

                    if (chatMsg.usernameLength == 6 && !strncmp(chatMsg.username, "SERVER", chatMsg.usernameLength)) {
                        chatText = TextFormat("[%s]<SERVER>: %.*s", "00:00:00", chatMsg.messageLength, chatMsg.message);
                        chatColor = RED;
                    } else {
                        chatText = TextFormat("[%s][%.*s]: %.*s", "00:00:00", chatMsg.usernameLength, chatMsg.username,
                            chatMsg.messageLength, chatMsg.message);
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
                if (GuiTextBoxAdvanced(&chatInputState, chatInputRect, chatInputText, &chatInputTextLen, CHAT_MESSAGE_LENGTH_MAX, io.WantCaptureKeyboard)) {
                    size_t messageLength = chatInputTextLen;
                    if (messageLength) {
                        netClient.SendChatMessage(chatInputText, messageLength);
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
            if (!chatActive && !io.WantCaptureKeyboard && IsKeyDown(KEY_T)) {
                chatActive = true;
                GuiSetActiveTextbox(&chatInputState);
            }
        }
#undef PUSH_TEXT

        rlDrawRenderBatchActive();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        EndDrawing();
        //----------------------------------------------------------------------------------

        // If nobody else handled the escape key, time to exit!
        if (escape) {
            break;
        }
    }
E_CLEANUP
    UnloadTexture(minimapTex);
    UnloadTexture(tilesetTex);
    UnloadTexture(checkboardTexture);
    sound_catalog_free();
    particles_free();
    loot_table_free();
    UnloadMusicStream(mus_background);
    UnloadMusicStream(mus_whistle);
    CloseAudioDevice();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CloseWindow();
    for (size_t i = 0; i < ARRAY_SIZE(fonts); i++) {
        UnloadFont(fonts[i]);
    }
E_END
}
