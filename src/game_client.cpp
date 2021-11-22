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

ErrorType GameClient::Run(const char *serverHost, unsigned short serverPort)
{
    const char *title = "Attack the slimes!";
    if (args.server) {
        title = "[Open to LAN] Attack the slimes!";
    }
    InitWindow(1600, 900, title);
    //InitWindow(600, 400, title);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);  // Disable default Escape exit key, we'll handle escape ourselves

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

    const int fontHeight = 14;
    Font font = LoadFontEx("C:/Windows/Fonts/consola.ttf", fontHeight, 0, 0);
    //font = LoadFontEx("resources/UbuntuMono-Regular.ttf", fontHeight, 0, 0);
    assert(font.texture.id);
    GuiSetFont(font);

    HealthBar::SetFont(GetFontDefault());

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);

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
    const float cameraSpeedDefault = 5.0f;
    float cameraSpeed = cameraSpeedDefault;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("ERROR: Failed to initialized audio device\n");
    }
    // NOTE: Minimum of 0.001 seems reasonable (0.0001 is still audible on max volume)
    //SetMasterVolume(0.01f);
    //SetMasterVolume(0.2f);

    DrawList::RegisterTypes();
    sound_catalog_init();
    SpritesheetCatalog::Load();
    tileset_init();

    DrawList drawList{};

    float mus_background_vmin = 0.02f;
    float mus_background_vmax = 0.2f;
    Music mus_background = LoadMusicStream("resources/fluquor_copyright.ogg");
    mus_background.looping = true;
    //PlayMusicStream(mus_background);
    SetMusicVolume(mus_background, mus_background_vmax);
    UpdateMusicStream(mus_background);

    float mus_whistle_vmin = 0.0f;
    float mus_whistle_vmax = 0.0f; //1.0f;
    Music mus_whistle = LoadMusicStream("resources/whistle.ogg");
    SetMusicVolume(mus_whistle, mus_whistle_vmin);
    mus_whistle.looping = true;

    float whistleAlpha = 0.0f;

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    World *lobby = new World;
    lobby->map = lobby->mapSystem.GenerateLobby();
    lobby->SpawnSam();

    {
        Player *player = lobby->SpawnPlayer(0);
        assert(player);
        lobby->playerId = player->id;
        player->combat.hitPoints = MAX(0, player->combat.hitPointsMax - 25);
    }

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
        rects[i].x = lobby->GetWorldSpawn().x + dlb_rand32f_variance_r(&rstar_rand, 600.0f);
        rects[i].y = lobby->GetWorldSpawn().y + dlb_rand32f_variance_r(&rstar_rand, 300.0f);
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

    const double dt = 1.0f / SERVER_TPS;
    // NOTE: Limit delta time to 2 frames worth of updates to prevent chaos for large dt (e.g. when debugging)
    const double dtMax = dt * 2;
    double frameStart = glfwGetTime();
    double frameAccum = 0.0f;
    double frameDt = 0.0f;

    const double inputDt = (1.0f / 60.0f);
    double inputAccum = 0.0f;

    const int targetFPS = 60;
    //SetTargetFPS(targetFPS);
    SetWindowState(FLAG_VSYNC_HINT);
    bool gifRecording = false;
    bool chatActive = false;

    bool show_demo_window = false;
    //---------------------------------------------------------------------------------------

#define KEY_DOWN(key)

    // Main game loop
    while (!WindowShouldClose()) {
        {
            double now = glfwGetTime();
            frameAccum += MIN(now - frameStart, dtMax);
            inputAccum += MIN(now - frameStart, dtMax);
            frameDt = now - frameStart;
            frameStart = now;
        }

        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            cameraReset = 1;
        }

        UpdateMusicStream(mus_background);
        UpdateMusicStream(mus_whistle);

        const bool imguiUsingMouse = io.WantCaptureMouse;
        const bool imguiUsingKeyboard = io.WantCaptureKeyboard;

        const bool processMouse = !imguiUsingMouse;
        const bool processKeyboard = !imguiUsingKeyboard && !chatActive;
        PlayerControllerState input = PlayerControllerState::Query(processMouse, processKeyboard, cameraFree);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        //ImGui::SetNextWindowSize(ImVec2(500, 0));
        if ((!netClient.server || netClient.server->state != ENET_PEER_STATE_CONNECTED) &&
            ImGui::BeginPopupModal("Log in", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char host[SERVER_HOST_LENGTH_MAX]{ "slime.theprogrammingjunkie.com" };
            static int  port = SERVER_PORT;
            static char username[USERNAME_LENGTH_MAX];
            static char password[PASSWORD_LENGTH_MAX];

            ImGui::Text("    Host:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(226);
            ImGui::InputText("##host", host, sizeof(host), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_ReadOnly);

            ImGui::Text("    Port:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(89);
            ImGui::InputInt("##port", &port, 1, 100, ImGuiInputTextFlags_ReadOnly);
            port = CLAMP(port, 0, USHRT_MAX);

            ImGui::Text("Username:");
            ImGui::SameLine();
            // https://github.com/ocornut/imgui/issues/455#issuecomment-167440172
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::SetNextItemWidth(226);
            ImGui::InputText("##username", username, sizeof(username));

            ImGui::Text("Password:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(226);
            ImGui::InputText("##password", password, sizeof(password), ImGuiInputTextFlags_Password);

            bool closePopup = false;

            ImGui::SetCursorPosX(177.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, 0xFFBF8346);
            bool login = ImGui::Button("Login", ImVec2(60, 0));
            ImGui::PopStyleColor();
            if (login ||
                IsKeyPressed(io.KeyMap[ImGuiKey_Enter]) ||
                IsKeyPressed(io.KeyMap[ImGuiKey_KeyPadEnter]))
            {
                E_ASSERT(netClient.Connect(host, (unsigned short)port, username, password), "Failed to connect to server");
                closePopup = true;
            }

            ImGui::SameLine();
            //ImGui::PushStyleColor(ImGuiCol_Button, 0xFF999999);
            bool cancel = ImGui::Button("Cancel", ImVec2(60, 0));
            //ImGui::PopStyleColor();
            if (cancel || IsKeyPressed(io.KeyMap[ImGuiKey_Escape])) {
                closePopup = true;
            }

            if (closePopup) {
                ImGui::CloseCurrentPopup();
                memset(username, 0, sizeof(username));
                memset(password, 0, sizeof(password));
                chatActive = false;
            }

            ImGui::EndPopup();
        }

        bool escape = IsKeyPressed(KEY_ESCAPE);
        bool findMouseTile = false;

        // HACK: No way to check if Raylib is currently recording.. :(
        //if (gifRecording) {
        //    dt = 1.0 / 10.0;
        //}

        E_ASSERT(netClient.Receive(), "Failed to receive packets");

        bool connectedToServer =
            netClient.server &&
            netClient.server->state == ENET_PEER_STATE_CONNECTED &&
            netClient.serverWorld &&
            netClient.serverWorld->playerId &&
            netClient.serverWorld->FindPlayer(netClient.serverWorld->playerId);

        if (connectedToServer) {
            // We joined a server, switch to the server's world.
            world = netClient.serverWorld;
        } else {
            world = lobby;
        }

        Player *playerPtr = world->FindPlayer(world->playerId);
        assert(playerPtr);
        Player &player = *playerPtr;

        if (connectedToServer) {
            if (netClient.worldHistory.Count()) {
                WorldSnapshot &worldSnapshot = netClient.worldHistory.Last();

                if (world->tick < worldSnapshot.tick) {
                    world->tick = worldSnapshot.tick;

                    // TODO: Update world state from worldSnapshot and re-apply input with input.tick > snapshot.tick
                    netClient.ReconcilePlayer();
                }

                //while (inputAccum > inputDt) {
                //
                //}

                InputSnapshot &inputSnapshot = netClient.inputHistory.Alloc();
                inputSnapshot.FromController(player.id, world->tick, frameDt, input);
                netClient.SendPlayerInput();
                player.ProcessInput(inputSnapshot);
                inputAccum -= inputDt;

                assert(world->map);
                player.Update(inputDt, *world->map);

                // TODO: Interpolate all of the other entities in the world
                double renderAt = glfwGetTime() - dt * 2;
                netClient.Interpolate(renderAt);

                world->particleSystem.Update(frameDt);
            }
        } else {
            InputSnapshot &inputSnapshot = netClient.inputHistory.Alloc();
            inputSnapshot.FromController(player.id, world->tick, frameDt, input);

#if 0
            while (frameAccum > dt) {
                world->Simulate(dt);
                world->tick++;
                frameAccum -= dt;
            }

            assert(world->map);
            player.ProcessInput(inputSnapshot);

            for (size_t i = 0; i < SERVER_PLAYERS_MAX; i++) {
                Player &p = world->players[i];
                if (!p.id) continue;
                p.Update(frameAccum, *world->map);
            }
            for (size_t i = 0; i < WORLD_ENTITIES_MAX; i++) {
                Slime &s = world->slimes[i];
                if (!s.id) continue;
                s.Update(frameAccum);
            }

            world->particleSystem.Update(frameAccum);
#else
            player.ProcessInput(inputSnapshot);
            world->Simulate(frameDt);
            world->particleSystem.Update(frameDt);
            world->tick++;
#endif
        }

        //if (!chatActive) {
            findMouseTile = input.dbgFindMouseTile;

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

            if (input.dbgImgui) {
                show_demo_window = !show_demo_window;
            }

            if (IsKeyPressed(KEY_F10)) {
                ToggleFullscreen();
            }

            if (input.dbgToggleVsync) {
                IsWindowState(FLAG_VSYNC_HINT) ? ClearWindowState(FLAG_VSYNC_HINT) : SetWindowState(FLAG_VSYNC_HINT);
            }

            if (input.dbgChatMessage) {
                netClient.SendChatMessage(CSTR("User pressed the C key."));
            }

            if (world == lobby && input.dbgSpawnSam) {
                lobby->SpawnSam();
            }

            if (input.dbgToggleFreecam) {
                cameraFree = !cameraFree;
            }
#if DEMO_VIEW_RTREE
            if (input.dbgNextRtreeRect) {
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
                camera.target = player.body.GroundPosition();
                camera.rotation = 0.0f;
                camera.zoom = 1.0f;
                cameraSpeed = cameraSpeedDefault;
            } else {
                cameraSpeed = CLAMP(cameraSpeed + input.cameraSpeedDelta, 1.0f, 50.0f);
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
        //}

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

        if (player.body.idle && whistleAlpha < 1.0f) {
            whistleAlpha = CLAMP(whistleAlpha + 0.005f, 0.0f, 1.0f);
            SetMusicVolume(mus_background, LERP(mus_background_vmin, mus_background_vmax, 1.0f - whistleAlpha));
            SetMusicVolume(mus_whistle, LERP(mus_whistle_vmin, mus_whistle_vmax, whistleAlpha));
            if (!IsMusicPlaying(mus_whistle)) {
                PlayMusicStream(mus_whistle);
            }
        } else if (!player.body.idle && whistleAlpha > 0.0f) {
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

        if (world->map->tiles) {
            const float tileWidthMip = (float)(TILE_W * zoomMipLevel);
            const float tileHeightMip = (float)(TILE_W * zoomMipLevel);
            const size_t height = world->map->height;
            const size_t width = world->map->width;
            const float camLeft = cameraRect.x;
            const float camTop = cameraRect.y;
            const float camRight = cameraRect.x + cameraRect.width;
            const float camBottom = cameraRect.y + cameraRect.height;

            for (size_t y = 0; y < height; y += zoomMipLevel) {
                for (size_t x = 0; x < width; x += zoomMipLevel) {
                    const Tile *tile = &world->map->tiles[y * world->map->width + x];
                    const Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                    if (tilePos.x + tileWidthMip >= camLeft &&
                        tilePos.y + tileHeightMip >= camTop &&
                        tilePos.x < camRight &&
                        tilePos.y < camBottom)                     {
                        // Draw all tiles as textured rects (looks best, performs worst)
                        Rectangle textureRect = tileset_tile_rect(world->map->tilesetId, tile->tileType);
                        tileset_draw_tile(world->map->tilesetId, tile->tileType, tilePos);
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
        int mouseTileX = 0;
        int mouseTileY = 0;
        if (findMouseTile) {
            mouseTile = world->map->TileAtWorldTry(mousePosWorld.x, mousePosWorld.y, &mouseTileX, &mouseTileY);
            if (mouseTile) {
                // Draw red outline on hovered tile
                Rectangle mouseTileRect{
                    (float)mouseTileX,
                    (float)mouseTileY,
                    (float)TILE_W * zoomMipLevel,
                    (float)TILE_W * zoomMipLevel
                };
                DrawRectangleLinesEx(mouseTileRect, 1 * (int)invZoom, RED);
            }
        }

        {
            drawList.EnableCulling(cameraRect);

            // Queue players for drawing
            for (size_t i = 0; i < ARRAY_SIZE(world->players); i++) {
                if (world->players[i].id) {
                    world->players[i].Push(drawList);

                }
            }

            // Queue slimes for drawing
            for (const Slime &slime : world->slimes) {
                if (slime.combat.hitPoints) {
                    slime.Push(drawList);
                }
            }

            // Queue particles for drawing
            world->particleSystem.Push(drawList);

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
            Vector3 charlieCenter = sprite_world_center(player.sprite, player.body.position, player.sprite.scale);
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
            DrawTextFont(font, TextFormat("tilePos : %.02f, %.02f", mouseTileX, mouseTileY),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawTextFont(font, TextFormat("tileSize: %zu, %zu", TILE_W, TILE_W),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawTextFont(font, TextFormat("tileType: %d", mouseTile->tileType),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
        }

        {
            // Render minimap
            const int minimapMargin = 6;
            const int minimapBorderWidth = 1;
            const int minimapX = screenWidth - minimapMargin - world->map->minimap.width - minimapBorderWidth * 2;
            const int minimapY = minimapMargin;
            const int minimapW = world->map->minimap.width + minimapBorderWidth * 2;
            const int minimapH = world->map->minimap.height + minimapBorderWidth * 2;
            const int minimapTexX = minimapX + minimapBorderWidth;
            const int minimapTexY = minimapY + minimapBorderWidth;
            DrawRectangleLines(minimapX, minimapY, minimapW, minimapH, BLACK);
            DrawTexture(world->map->minimap, minimapTexX, minimapTexY, WHITE);

            // Draw slimes on map
            for (size_t i = 0; i < ARRAY_SIZE(world->slimes); i++) {
                Slime &s = world->slimes[i];
                if (s.id) {
                    float x = (s.body.position.x / (world->map->width * TILE_W)) * minimapW + minimapX;
                    float y = (s.body.position.y / (world->map->height * TILE_W)) * minimapH + minimapY;
                    DrawCircle((int)x, (int)y, 2.0f, Color{ 0, 170, 80, 255 });
                }
            }

            // Draw players on map
            for (size_t i = 0; i < ARRAY_SIZE(world->players); i++) {
                Player &p = world->players[i];
                if (p.id) {
                    float x = (p.body.position.x / (world->map->width  * TILE_W)) * minimapW + minimapX;
                    float y = (p.body.position.y / (world->map->height * TILE_W)) * minimapH + minimapY;
                    const Color playerColor{ 220, 90, 20, 255 };
                    DrawCircle((int)x, (int)y, 2.0f, playerColor);
                    const char *pName = TextFormat("%.*s", p.nameLength, p.name);
                    int nameWidth = MeasureText(pName, fontHeight);
                    DrawTextFont(font, pName, x - nameWidth / 2, y - fontHeight - 4, fontHeight, YELLOW);
                }
            }
        }

        const char *text = 0;
        float hudCursorY = 0;

#define PUSH_TEXT(text, color) \
    DrawTextFont(font, text, margin + pad, hudCursorY, fontHeight, color); \
    hudCursorY += fontHeight + pad; \

        // Render HUD
        {
            int linesOfText = 8;
#if SHOW_DEBUG_STATS
            linesOfText += 10;
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
            text = TextFormat("Coins: %d", player.inventory.slots[(int)PlayerInventorySlot::Coins].stackCount);
            PUSH_TEXT(text, YELLOW);

            text = TextFormat("Coins collected   %u", player.stats.coinsCollected);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Damage dealt      %.2f", player.stats.damageDealt);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Kilometers walked %.2f", player.stats.kmWalked);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Slimes slain      %u", player.stats.slimesSlain);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Times fist swung  %u", player.stats.timesFistSwung);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Times sword swung %u", player.stats.timesSwordSwung);
            PUSH_TEXT(text, LIGHTGRAY);

#if SHOW_DEBUG_STATS
            text = TextFormat("Tick          %u", world->tick);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("framAccum     %.03f", frameAccum);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("frameDt       %.03f", frameDt);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Camera speed  %.03f", cameraSpeed);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Zoom          %.03f", camera.zoom);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Zoom inverse  %.03f", invZoom);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Zoom mip      %zu", zoomMipLevel);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Tiles visible %zu", tilesDrawn);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Particle FX   %zu", world->particleSystem.EffectsActive());
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Particles     %zu", world->particleSystem.ParticlesActive());
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
#undef PUSH_TEXT

        {
            const float margin = 6.0f;   // left/bottom margin
            const float pad = 4.0f;      // left/bottom pad
            const float inputBoxHeight = font.baseSize + pad * 2.0f;
            const int linesOfText = (int)world->chatHistory.MessageCount();
            const float chatWidth = 800.0f;
            const float chatHeight = linesOfText * (font.baseSize + pad) + pad;
            const float chatX = margin;
            const float chatY = screenHeight - margin - inputBoxHeight - chatHeight;

            // Render chat history
            world->chatHistory.Render(font, { chatX, chatY, chatWidth, chatHeight });

            // Render chat input box
            static GuiTextBoxAdvancedState chatInputState;
            if (chatActive) {
                static int chatInputTextLen = 0;
                static char chatInputText[CHAT_MESSAGE_LENGTH_MAX];

                Rectangle chatInputRect = { margin, screenHeight - margin - inputBoxHeight, chatWidth, inputBoxHeight };
                //GuiTextBox(inputBox, chatInputText, CHAT_MESSAGE_LENGTH_MAX, true);
                //GuiTextBoxEx(inputBox, chatInputText, CHAT_MESSAGE_LENGTH_MAX, true);
                if (GuiTextBoxAdvanced(&chatInputState, chatInputRect, chatInputText, &chatInputTextLen, CHAT_MESSAGE_LENGTH_MAX, io.WantCaptureKeyboard)) {
                    size_t messageLength = chatInputTextLen;
                    if (messageLength) {
                        ErrorType sendResult = netClient.SendChatMessage(chatInputText, messageLength);
                        switch (sendResult) {
                        case ErrorType::NotConnected:
                        {
                            world->chatHistory.PushMessage(CSTR("Sam"), CSTR("You're not connected to a server. Nobody is listening. :("));
                            ImGui::OpenPopup("Log in");
                            break;
                        }
                        }
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

        rlDrawRenderBatchActive();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        EndDrawing();
        //----------------------------------------------------------------------------------

        if (connectedToServer && escape) {
            netClient.Disconnect();
            world = lobby;
            escape = false;
        }

        // If nobody else handled the escape key, time to exit!
        if (escape) {
            break;
        }
    }

    netClient.CloseSocket();

    // TODO: Wrap these in classes to use destructors?
    delete lobby;
    UnloadTexture(checkboardTexture);
    UnloadMusicStream(mus_background);
    UnloadMusicStream(mus_whistle);
    UnloadFont(font);
    sound_catalog_free();
    CloseAudioDevice();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CloseWindow();
    return ErrorType::Success;
}
