#include "draw_command.h"
#include "game_client.h"
#include "healthbar.h"
#include "catalog/items.h"
#include "loot_table.h"
#include "net_client.h"
#include "particles.h"
#include "rtree.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "catalog/tracks.h"
#include "structures/structure.h"
#include "ui/ui.h"
#include "dlb_types.h"

#include "raylib/raylib.h"
#include "raylib/raygui.h"
//#include "gui_textbox_extended.h"
#define GRAPHICS_API_OPENGL_33
#include "raylib/rlgl.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"
#include "GLFW/glfw3.h"

const char *GameClient::LOG_SRC = "GameClient";

ErrorType GameClient::Run(void)
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

    Vector2 cameraGoal = { screenWidth / 2.0f, screenHeight / 2.0f };
    bool cameraReset = false;
    bool cameraFree = false;
    const float cameraSpeedDefault = 5.0f;
    float cameraSpeed = cameraSpeedDefault;
    Camera2D camera{};
    //camera.target = (Vector2){
    //    (float)tilemap.width / 2.0f * tilemap.tileset->tileWidth,
    //    (float)tilemap.height / 2.0f * tilemap.tileset->tileHeight
    //};
    camera.offset = cameraGoal;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("ERROR: Failed to initialized audio device\n");
    }

    DrawList::RegisterTypes();
    Catalog::g_items.Load();
    Catalog::g_mixer.Load();
    Catalog::g_particleFx.Load();
    Catalog::g_sounds.Load();
    Catalog::g_spritesheets.Load();
    Catalog::g_tracks.Load();
    tileset_init();

    Catalog::g_mixer.masterVolume = 0.5f;

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    World *lobby = new World;
    lobby->tick = 1;
    lobby->map = lobby->mapSystem.GenerateLobby();
    Slime &sam = lobby->SpawnSam();
    world = lobby;

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

    const double tickDt = 1.0 / SV_TICK_RATE;
    const double tickDtMax = tickDt * 2.0;
    double tickAccum = 0;
    const double sendInputDt = 1.0 / CL_INPUT_SEND_RATE;
    const double sendInputDtMax = sendInputDt * 2.0;
    double sendInputAccum = 0;
    double frameStart = glfwGetTime();

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
        double now = glfwGetTime();
        double frameDt = now - frameStart;
        // Limit delta time to prevent spiraling for after long hitches (e.g. hitting a breakpoint)
        tickAccum += MIN(frameDt, tickDtMax);
        sendInputAccum += MIN(sendInputDt, sendInputDtMax);
        frameStart = now;

        //----------------------------------------------------------------------
        // Audio
        //----------------------------------------------------------------------
        SetMasterVolume(VolumeCorrection(Catalog::g_mixer.masterVolume));
        Catalog::g_tracks.Update((float)frameDt);
        //----------------------------------------------------------------------

        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            cameraReset = 1;
        }

        const bool imguiUsingMouse = io.WantCaptureMouse;
        const bool imguiUsingKeyboard = io.WantCaptureKeyboard;

        const bool processMouse = !imguiUsingMouse;
        const bool processKeyboard = !imguiUsingKeyboard && !chatActive;
        PlayerControllerState input = PlayerControllerState::Query(processMouse, processKeyboard, cameraFree);
        bool escape = IsKeyPressed(KEY_ESCAPE);
        bool findMouseTile = false;

        Player *playerPtr = world->FindPlayer(world->playerId);
        assert(playerPtr);
        Player &player = *playerPtr;

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
            cameraGoal = player.body.GroundPosition();
            camera.rotation = 0.0f;
            camera.zoom = 1.0f;
            cameraSpeed = cameraSpeedDefault;
        } else {
            cameraSpeed = CLAMP(cameraSpeed + input.cameraSpeedDelta, 1.0f, 50.0f);
            if (input.cameraWest)  cameraGoal.x -= cameraSpeed / camera.zoom;
            if (input.cameraEast)  cameraGoal.x += cameraSpeed / camera.zoom;
            if (input.cameraNorth) cameraGoal.y -= cameraSpeed / camera.zoom;
            if (input.cameraSouth) cameraGoal.y += cameraSpeed / camera.zoom;
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
        camera.target = v2_add(camera.target, v2_scale(v2_sub(cameraGoal, camera.target), 1.0f));

        const int negZoomMultiplier = 7; // 7x negative zoom (out)
        const int posZoomMultiplier = 1; // 2x positive zoom (in)
#if 1
        const float minZoom = 1.0f / (float)(1 << (negZoomMultiplier - 1));
        const float maxZoom = 1.0f * (float)(1 << (posZoomMultiplier - 1));
        camera.zoom = CLAMP(camera.zoom, minZoom, maxZoom);
#else
        const float minZoomPow2 = (float)(1 << (negZoomMultiplier - 1));
        const float maxZoomPow2 = (float)(1 << (posZoomMultiplier - 1));
        const float minZoom = 1.0f / minZoomPow2;
        const float maxZoom = 1.0f * maxZoomPow2;
        const float roundedZoom = roundf(camera.zoom * minZoomPow2) / minZoomPow2;
        camera.zoom = CLAMP(roundedZoom, minZoom, maxZoom);
#endif
        const float invZoom = 1.0f / camera.zoom;
        const int zoomMipLevel = ZoomMipLevel(invZoom);

        Rectangle cameraRect{};
        cameraRect.x = camera.target.x - camera.offset.x * invZoom;
        cameraRect.y = camera.target.y - camera.offset.y * invZoom;
        cameraRect.width = camera.offset.x * 2.0f * invZoom;
        cameraRect.height = camera.offset.y * 2.0f * invZoom;

        const Vector2 mousePosScreen = GetMousePosition();
        Vector2 mousePosWorld{};
        mousePosWorld.x += cameraRect.x + mousePosScreen.x * invZoom;
        mousePosWorld.y += cameraRect.y + mousePosScreen.y * invZoom;

        // HACK: No way to check if Raylib is currently recording.. :(
        //if (gifRecording) {
        //    tickDt = 1.0 / 10.0;
        //}

        E_ASSERT(netClient.Receive(), "Failed to receive packets");

        bool connectedToServer =
            netClient.server &&
            netClient.server->state == ENET_PEER_STATE_CONNECTED &&
            netClient.serverWorld &&
            netClient.serverWorld->playerId &&
            netClient.serverWorld->FindPlayer(netClient.serverWorld->playerId);

        if (connectedToServer && world != netClient.serverWorld) {
            // We joined a server, switch to the server's world.
            world = netClient.serverWorld;
            tickAccum = 0;
            sendInputAccum = 0;
        } else if (!connectedToServer && world != lobby ) {
            world = lobby;
            tickAccum = 0;
            sendInputAccum = 0;
        }

        if (connectedToServer) {
            if (netClient.worldHistory.Count()) {
                WorldSnapshot &worldSnapshot = netClient.worldHistory.Last();

                static uint32_t lastTickAck = 0;
                if (lastTickAck < worldSnapshot.tick) {
                    lastTickAck = worldSnapshot.tick;

                    // TODO: Update world state from worldSnapshot and re-apply input with input.tick > snapshot.tick
                    netClient.ReconcilePlayer(tickDt);
                }
                // TODO: Update world state from worldSnapshot and re-apply input with input.tick > snapshot.tick
                //netClient.ReconcilePlayer(tickDt);

                //while (tickAccum > tickDt) {
                    InputSample &inputSample = netClient.inputHistory.Alloc();
                    inputSample.FromController(player.id, lastTickAck, input);

                    assert(world->map);
                    player.Update(tickDt, inputSample, *world->map);
                    world->particleSystem.Update(tickDt);

                    tickAccum -= tickDt;
                //}

                // Send input to server at a fixed rate that matches server tick rate
                while (sendInputAccum > sendInputDt) {
                    netClient.SendPlayerInput();
                    sendInputAccum -= sendInputDt;
                }

                // Interpolate all of the other entities in the world
                //double renderAt = glfwGetTime() - ((1000.0 / (SNAPSHOT_SEND_RATE * 5.0)) / 1000.0);
                //double renderAt = glfwGetTime() - ((1.0 / SNAPSHOT_SEND_RATE) * 2.0 + (1.0 / netClient.server->lastRoundTripTime) * 2.0);
                double renderAt = glfwGetTime() - (100.0 / 1000.0);
                //double renderAt = worldSnapshot.recvAt;
                world->Interpolate(renderAt);
            }
        } else {
            while (tickAccum > tickDt) {
                InputSample inputSample{};
                inputSample.FromController(player.id, world->tick, input);

                assert(world->map);
                player.Update(tickDt, inputSample, *world->map);

                world->Simulate(tickDt);
                world->particleSystem.Update(tickDt);

                WorldSnapshot &worldSnapshot = netClient.worldHistory.Alloc();
                assert(!worldSnapshot.tick);  // ringbuffer alloc fucked up and didn't zero slot
                worldSnapshot.playerId = player.id;
                //worldSnapshot.lastInputAck = world->tick;
                worldSnapshot.tick = world->tick;
                world->GenerateSnapshot(worldSnapshot);

                world->tick++;
                tickAccum -= tickDt;
            }

            // Interpolate all of the other entities in the world
            double renderAt = glfwGetTime() - tickDt * 2;
            world->Interpolate(renderAt);
        }

        static bool samTreasureRoom = false;
        if (!samTreasureRoom && world == lobby && !sam.combat.hitPoints) {
            uint32_t chestX = MAX(0, int(player.body.position.x / TILE_W));
            uint32_t chestY = MAX(0, int(player.body.position.y / TILE_W) - 2);
            Structure::Spawn(*world->map, chestX - 3, chestY - 4);
            Vector3 chestPos{ (chestX * TILE_W) + TILE_W * 0.5f, float(chestY * TILE_W), 0.0f };
            world->particleSystem.GenerateEffect(Catalog::ParticleEffectID::GoldenChest, 1, chestPos, 4.0f);
            world->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Gem, 16, chestPos, 4.0f);
            Catalog::g_sounds.Play(Catalog::SoundID::Gold, 1.0f + dlb_rand32f_variance(0.4f));
            samTreasureRoom = true;
        }

        //if (!chatActive) {
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
                samTreasureRoom = false;
            }

            if (input.dbgToggleFreecam) {
                cameraFree = !cameraFree;
            }
#if DEMO_VIEW_RTREE
            if (input.dbgNextRtreeRect) {
                if (next_rect_to_add < RECT_COUNT && (GetTime() - lastRectAddedAt > 0.1)) {
                    AABB aabb{};
                    aabb.min.x = rects[next_rect_to_add].x;
                    aabb.min.y = rects[next_rect_to_add].y;
                    aabb.max.x = rects[next_rect_to_add].x + rects[next_rect_to_add].width;
                    aabb.max.y = rects[next_rect_to_add].y + rects[next_rect_to_add].height;
                    tree.Insert(aabb, next_rect_to_add);
                    next_rect_to_add++;
                    lastRectAddedAt = GetTime();
                }
            } else {
                lastRectAddedAt = 0;
            }

            if (input.dbgKillRtreeRect) {
                size_t lastIdx = next_rect_to_add - 1;
                AABB aabb{};
                aabb.min.x = rects[lastIdx].x;
                aabb.min.y = rects[lastIdx].y;
                aabb.max.x = rects[lastIdx].x + rects[lastIdx].width;
                aabb.max.y = rects[lastIdx].y + rects[lastIdx].height;
                tree.Delete(aabb, lastIdx);
                next_rect_to_add--;
            }
#endif

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        UI::Begin(font, invZoom, mousePosScreen, mousePosWorld, { (float)screenWidth, (float)screenHeight });

        ClearBackground(ORANGE);
        DrawTexture(checkboardTexture, 0, 0, WHITE);

        BeginMode2D(camera);

        world->EnableCulling(cameraRect);
        size_t tilesDrawn = world->DrawMap(zoomMipLevel);
        if (input.dbgFindMouseTile) {
            UI::TileHoverOutline(*world->map);
        }
        world->DrawEntities();
        world->DrawParticles();
        world->DrawFlush();

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

#if DEMO_AI_TRACKING
        {
            for (size_t i = 0; i < ARRAY_SIZE(world->slimes); i++) {
                const Slime &slime = world->slimes[i];
                if (slime.id) {
                    Vector3 center = sprite_world_center(slime.sprite, slime.body.position, slime.sprite.scale);
                    DrawCircle((int)center.x, (int)center.y, METERS_TO_PIXELS(10.0f), Fade(ORANGE, 0.3f));
                }
            }
        }
#endif
        //UI::WorldGrid(*world->map);

        EndMode2D();

        UI::Minimap(*world);

        // Render HUD
        UI::DebugStats debugStats{};
#if SHOW_DEBUG_STATS
        debugStats.tick = world->tick;
        debugStats.tickAccum = tickAccum;
        debugStats.frameDt = frameDt;
        if (netClient.server) {
            debugStats.ping = netClient.server->lastRoundTripTime;
        }
        debugStats.cameraSpeed = cameraSpeed;
        debugStats.tilesDrawn = tilesDrawn;
        debugStats.effectsActive = world->particleSystem.EffectsActive();
        debugStats.particlesActive = world->particleSystem.ParticlesActive();
#endif
        UI::HUD(player, debugStats);

        {
            const float margin = 6.0f;   // left/bottom margin
            const float pad = 4.0f;      // left/bottom pad
            const float inputBoxHeight = font.baseSize + pad * 2.0f;
            const float chatWidth = 800.0f;
            const float chatBottom = screenHeight - margin - inputBoxHeight;

            // Render chat history
            world->chatHistory.Render(font, margin, chatBottom, chatWidth, chatActive);

            // Render chat input box
            static GuiTextBoxAdvancedState chatInputState;
            if (chatActive) {
                static int chatInputTextLen = 0;
                static char chatInputText[CHATMSG_LENGTH_MAX]{};

                Rectangle chatInputRect = { margin, screenHeight - margin - inputBoxHeight, chatWidth, inputBoxHeight };
                if (GuiTextBoxAdvanced(&chatInputState, chatInputRect, chatInputText, &chatInputTextLen, CHATMSG_LENGTH_MAX, io.WantCaptureKeyboard)) {
                    if (chatInputTextLen) {
                        ErrorType sendResult = netClient.SendChatMessage(chatInputText, chatInputTextLen);
                        switch (sendResult) {
                            case ErrorType::NotConnected:
                            {
                                world->chatHistory.PushMessage(CSTR("Sam"), CSTR("You're not connected to a server. Nobody is listening. :("));
                                break;
                            }
                        }
                        chatActive = false;
                        memset(chatInputText, 0, sizeof(chatInputText));
                        chatInputTextLen = 0;
                    }
                }

                if (escape) {
                    chatActive = false;
                    memset(chatInputText, 0, sizeof(chatInputText));
                    chatInputTextLen = 0;
                    escape = false;
                }
            }

            // HACK: Check for this *after* rendering chat to avoid pressing "t" causing it to show up in the chat box
            if (!chatActive && !io.WantCaptureKeyboard && IsKeyDown(KEY_T)) {
                chatActive = true;
                GuiSetActiveTextbox(&chatInputState);
            }
        }

        // Render mouse tile tooltip
        if (input.dbgFindMouseTile) {
            UI::TileHoverTip(*world->map);
        }

        rlDrawRenderBatchActive();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        UI::LoginForm(netClient, io, escape);
        UI::Mixer();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        EndDrawing();
        //----------------------------------------------------------------------------------

        if (escape) {
            if (connectedToServer) {
                netClient.Disconnect();
                world = lobby;
                escape = false;
            } else {
                // If nobody else handled the escape key, time to exit!
                break;
            }
        }
    }

    netClient.CloseSocket();

    // TODO: Wrap these in classes to use destructors?
    delete lobby;
    UnloadTexture(checkboardTexture);
    UnloadFont(font);
    Catalog::g_tracks.Unload();
    Catalog::g_sounds.Unload();
    CloseAudioDevice();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CloseWindow();
    return ErrorType::Success;
}
