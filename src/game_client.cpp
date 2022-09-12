#include "catalog/items.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "catalog/tracks.h"
#include "clock.h"
#include "draw_command.h"
#include "catalog/fonts.h"
#include "fx/fx.h"
#include "game_client.h"
#include "healthbar.h"
#include "loot_table.h"
#include "net_client.h"
#include "particles.h"
#include "rtree.h"
#include "spycam.h"
#include "structures/structure.h"
#include "ui/ui.h"
#include "dlb_types.h"

#include "raylib/raylib.h"
#define GRAPHICS_API_OPENGL_33
#include "raylib/rlgl.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"
#include "GLFW/glfw3.h"

const char *GameClient::LOG_SRC = "GameClient";

// TODO: Move this somewhere less stupid (e.g. a library of particle updaters)
inline void RainbowParticlesDamagePlayer(Particle &particle, void *userData)
{
    assert(userData);

    Player *player = (Player *)userData;
    const Vector3 particleWorld = v3_add(particle.effect->origin, particle.body.WorldPosition());
    const Vector3 gut = player->GetAttachPoint(Player::AttachPoint::Gut);
    const float particleToGut = v3_length_sq(v3_sub(gut, particleWorld));
    if (particleToGut < SQUARED(METERS_TO_PIXELS(0.2f))) {
        player->combat.DealDamage(0.1f);
    }
}

void GameClient::LoadingScreen(const char *text)
{
    assert(checkboardTexture.id);
    assert(screenSize.x);
    assert(g_fonts.fontBig.texture.id);

    BeginDrawing();
    DrawTexture(checkboardTexture, 0, 0, WHITE);
    DrawRectangle(0, 0, (int)screenSize.x, (int)screenSize.y, Fade(BLACK, 0.5f));
    Vector2 size = MeasureTextEx(g_fonts.fontBig, text, (float)g_fonts.fontBig.baseSize, 1.0f);
    Vector2 pos{ screenSize.x / 2.0f - screenSize.x / 3.0f, screenSize.y / 2.0f - size.y / 2.0f };
    DrawTextFont(g_fonts.fontBig, text, pos.x, pos.y, -2, -2, g_fonts.fontBig.baseSize, WHITE);
    EndDrawing();
}

void GameClient::Init(void)
{
    InitWindow(1600, 900, "Attack the slimes!");
    // NOTE: I could avoid the flicker if Raylib would let me pass it as a flag into InitWindow -_-
    //SetWindowState(FLAG_WINDOW_HIDDEN);

    SetExitKey(0);  // Disable default Escape exit key, we'll handle escape ourselves
    screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    //const int targetFPS = 60;
    //SetTargetFPS(targetFPS);
    SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    const char *fontName = "C:/Windows/Fonts/consolab.ttf";
    //const char *fontName = "data/font/Hack-Bold.ttf";
    //const char *fontName = "data/font/PressStart2P-vaV7.ttf";
    g_fonts.fontSmall = LoadFontEx(fontName, 16, 0, 0);
    assert(g_fonts.fontSmall.texture.id);
    g_fonts.fontBig = LoadFontEx(fontName, 72, 0, 0);
    assert(g_fonts.fontBig.texture.id);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    // Setup Dear ImGui context
    LoadingScreen("Loading UI...");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();  // Setup Dear ImGui style

    // Setup Platform/Renderer backends
    LoadingScreen("Loading Renderer...");
    GLFWwindow *glfwWindow = glfwGetCurrentContext();
    assert(glfwWindow);
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    HealthBar::SetFont(g_fonts.fontSmall);

    LoadingScreen("Loading Shaders...");
#if CL_PIXEL_FIXER
    pixelFixer                     = LoadShader("data/shader/pixelfixer.vs", "data/shader/pixelfixer.fs");
    pixelFixerScreenSizeUniformLoc = GetShaderLocation(pixelFixer, "screenSize");
    SetShaderValue(pixelFixer, pixelFixerScreenSizeUniformLoc, &screenSize, SHADER_UNIFORM_VEC2);
#endif

    LoadingScreen("Loading Fonts...");
    ImGuiIO &io = ImGui::GetIO();
    g_fonts.imFontHack16 = io.Fonts->AddFontFromFileTTF(fontName, 16.0f);
    g_fonts.imFontHack32 = io.Fonts->AddFontFromFileTTF(fontName, 32.0f);
    g_fonts.imFontHack48 = io.Fonts->AddFontFromFileTTF(fontName, 48.0f);
    g_fonts.imFontHack64 = io.Fonts->AddFontFromFileTTF(fontName, 64.0f);
    io.FontDefault = g_fonts.imFontHack16;

    // SDF font generation from TTF font
    {
        unsigned int fileSize = 0;
        unsigned char *fileData = 0;
        Image atlas{};

        g_fonts.fontSdf24.baseSize = 24;
        g_fonts.fontSdf24.glyphCount = 95;
        // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 0 (defaults to 95)
        // Loading file to memory
        fileData = LoadFileData(fontName, &fileSize);
        g_fonts.fontSdf24.glyphs = LoadFontData(fileData, fileSize, g_fonts.fontSdf24.baseSize, 0, 0, FONT_SDF);
        // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 0 px, pack method: 1 (Skyline algorythm)
        atlas = GenImageFontAtlas(g_fonts.fontSdf24.glyphs, &g_fonts.fontSdf24.recs, 95, g_fonts.fontSdf24.baseSize, 0, 1);
        g_fonts.fontSdf24.texture = LoadTextureFromImage(atlas);
        UnloadImage(atlas);
        UnloadFileData(fileData);  // Free memory from loaded file

        g_fonts.fontSdf72.baseSize = 72;
        g_fonts.fontSdf72.glyphCount = 95;
        // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 0 (defaults to 95)
        // Loading file to memory
        fileData = LoadFileData(fontName, &fileSize);
        g_fonts.fontSdf72.glyphs = LoadFontData(fileData, fileSize, g_fonts.fontSdf72.baseSize, 0, 0, FONT_SDF);
        // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 0 px, pack method: 1 (Skyline algorythm)
        atlas = GenImageFontAtlas(g_fonts.fontSdf72.glyphs, &g_fonts.fontSdf72.recs, 95, g_fonts.fontSdf72.baseSize, 0, 1);
        g_fonts.fontSdf72.texture = LoadTextureFromImage(atlas);
        UnloadImage(atlas);
        UnloadFileData(fileData);  // Free memory from loaded file

        // Load SDF required shader (we use default vertex shader)
        g_sdfShader = LoadShader(0, "data/font/sdf.fs");
        SetTextureFilter(g_fonts.fontSdf72.texture, TEXTURE_FILTER_BILINEAR);  // Required for SDF font
    }

    LoadingScreen("Loading Cameras...");
    g_spycam.Init({ screenSize.x * 0.5f, screenSize.y * 0.5f });

    LoadingScreen("Loading Audio Devices...");
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("ERROR: Failed to initialized audio device\n");
    }

    LoadingScreen("Loading Items...");
    g_item_catalog.LoadTextures();
    g_item_catalog.LoadData();

    LoadingScreen("Loading Particles...");
    Catalog::g_particleFx.Load();

    LoadingScreen("Loading Sounds...");
    Catalog::g_sounds.Load();
    LoadingScreen("Loading Music...");
    Catalog::g_tracks.Load();
    Catalog::g_mixer.masterVolume = 0.75f;
    Catalog::g_mixer.musicVolume = 0.3f;
    Catalog::g_sounds.mixer.volumeLimit[(size_t)Catalog::SoundID::GemBounce] = 0.8f;
    Catalog::g_sounds.mixer.volumeLimit[(size_t)Catalog::SoundID::Whoosh] = 0.6f;

    LoadingScreen("Loading Spritesheets...");
    Catalog::g_spritesheets.Load();
    LoadingScreen("Loading Tilesets...");
    tileset_init();

#if CL_DEMO_VIEW_RTREE
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
}

void GameClient::PlayMode_PollController(PlayerControllerState &input)
{
    ImGuiIO &io = ImGui::GetIO();
    const bool processKeyboard = !io.WantCaptureKeyboard;
    const bool processMouse = !io.WantCaptureKeyboard && !io.WantCaptureMouse;
    input.Query(processMouse, processKeyboard, g_spycam.freeRoam);
}

ErrorType GameClient::PlayMode_Network()
{
    E_ASSERT(netClient.Receive(), "Failed to receive packets");

    if (UI::DisconnectRequested(netClient.IsDisconnected())) {
        netClient.Disconnect();
    }

    // If I disconnect from local server for any reason, clean up the thread
    if (localServer && netClient.IsDisconnected()) {
        args.serverQuit = true;
        delete localServer;
        localServer = 0;
        args.serverQuit = false;
    }

    return ErrorType::Success;
}

void GameClient::PlayMode_Audio(double frameDt)
{
    SetMasterVolume(VolumeCorrection(Catalog::g_mixer.masterVolume));
    Catalog::g_tracks.Update((float)frameDt);
}

void GameClient::PlayMode_HandleInput(PlayerControllerState &input)
{
    if (IsWindowResized() || input.toggleFullscreen) {
        if (IsWindowResized()) {
            screenSize.x = (float)GetRenderWidth() - GetRenderWidth() % 2;
            screenSize.y = (float)GetRenderHeight() - GetRenderHeight() % 2;
            SetWindowSize((int)screenSize.x, (int)screenSize.y);
        }
        if (input.toggleFullscreen) {
            thread_local Vector2 restoreSize = screenSize;
            if (IsWindowFullscreen()) {
                ToggleFullscreen();
                SetWindowSize((int)restoreSize.x, (int)restoreSize.y);
                screenSize = restoreSize;
            } else {
                restoreSize = screenSize;
                int monitor = GetCurrentMonitor();
                screenSize.x = (float)GetMonitorWidth(monitor);
                screenSize.y = (float)GetMonitorHeight(monitor);
                SetWindowSize((int)screenSize.x, (int)screenSize.y);
                ToggleFullscreen();
            }
        }
        // Weird line artifact appears, even with AlignPixel shader, if resolution has an odd number :(
        assert((int)screenSize.x % 2 == 0);
        assert((int)screenSize.y % 2 == 0);
        //printf("w: %f h: %f\n", screenSize.x, screenSize.y);
#if CL_PIXEL_FIXER
        SetShaderValue(pixelFixer, pixelFixerScreenSizeUniformLoc, &screenSize, SHADER_UNIFORM_VEC2);
#endif
        g_spycam.Reset();
    }

    if (input.dbgToggleVsync) {
        IsWindowState(FLAG_VSYNC_HINT) ? ClearWindowState(FLAG_VSYNC_HINT) : SetWindowState(FLAG_VSYNC_HINT);
    }

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

    if (input.toggleInventory) {
        inventoryActive = !inventoryActive;
    }

    if (input.dbgChatMessage) {
        netClient.serverWorld->chatHistory.PushDebug(CSTR("You pressed the send random chat message button. Congrats."));
        g_clock.now += SV_TICK_DT * 100;
    }

    if (input.dbgTeleport) {
        netClient.SendChatMessage(CSTR("/rtp"));
    }

#if CL_DEMO_VIEW_RTREE
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
}

void GameClient::PlayMode_Update(double frameDt, PlayerControllerState &input)
{
    Player *player = netClient.serverWorld->FindPlayer(netClient.serverWorld->playerId);

    renderAt = 0;
#if 0
    while (tickAccum > tickDt) {
#endif
        if (netClient.worldHistory.Count()) {
            const WorldSnapshot &worldSnapshot = netClient.worldHistory.Last();
            //printf("worldTick: %u, lastSnapshot: %u, delta: %u\n", world->tick, worldSnapshot.tick, worldSnapshot.tick - world->tick);
            netClient.serverWorld->tick = worldSnapshot.tick;
            g_clock.serverNow = worldSnapshot.clock;

            //while (tickAccum > tickDt) {
            netClient.inputSeq = MAX(1, netClient.inputSeq + 1);
            InputSample &inputSample = netClient.inputHistory.Alloc();
            inputSample.FromController(player->id, netClient.inputSeq, frameDt, input);

            // Update world state from worldSnapshot and re-apply input with input.tick > snapshot.tick
            netClient.ReconcilePlayer();

            player->Update(inputSample, netClient.serverWorld->map);
            netClient.serverWorld->particleSystem.Update(frameDt);
            netClient.serverWorld->itemSystem.Update(frameDt);

            //    tickAccum -= tickDt;
            //}

            // Send queued inputs to server ASAP (ideally, every frame), while respecting a sane rate limit
            if (g_clock.now - netClient.lastInputSentAt > CL_INPUT_SEND_RATE_LIMIT_DT) {
                netClient.SendPlayerInput();
                netClient.lastInputSentAt = g_clock.now;
            }

            netClient.serverWorld->CL_DespawnStaleEntities();

            // Interpolate all of the other entities in the world
            //printf("RTT: %u RTTv: %u LRT: %u LRTv: %u HRTv %u\n",
            //    netClient.server->roundTripTime,
            //    netClient.server->roundTripTimeVariance,
            //    netClient.server->lastRoundTripTime,
            //    netClient.server->lastRoundTripTimeVariance,
            //    netClient.server->highestRoundTripTimeVariance
            //);
            //renderAt = g_clock.now - (1.0 / SNAPSHOT_SEND_RATE) - (1.0 / (netClient.server->lastRoundTripTime + netClient.server->lastRoundTripTimeVariance));
            renderAt = g_clock.now - (1.0 / (SNAPSHOT_SEND_RATE * 1.5));
            netClient.serverWorld->CL_Interpolate(renderAt);
            //netClient.serverWorld->CL_Extrapolate(g_clock.now - renderAt);
            netClient.serverWorld->CL_Animate(frameDt);
        }
#if 0
        tickAccum -= tickDt;
    }
#endif
}

void GameClient::PlayMode_UpdateCamera(double frameDt, PlayerControllerState &input)
{
    if (!g_spycam.freeRoam) {
        Player *player = netClient.serverWorld->FindPlayer(netClient.serverWorld->playerId);
        assert(player);
        Vector2 wtc = player->WorldTopCenter2D();
        g_spycam.cameraGoal = { wtc.x, wtc.y };
    }
    g_spycam.Update(input, frameDt);
}

void GameClient::PlayMode_DrawWorld(PlayerControllerState &input)
{
    Camera2D cam = g_spycam.GetCamera();
    BeginMode2D(cam);

    Rectangle cameraRect = g_spycam.GetRect();
    netClient.serverWorld->EnableCulling(cameraRect);

#if CL_PIXEL_FIXER
    BeginShaderMode(pixelFixer);
#endif
    tilesDrawn = netClient.serverWorld->DrawMap(g_spycam);
#if CL_PIXEL_FIXER
    EndShaderMode();
#endif

    //UI::WorldGrid();

    if (input.dbgFindMouseTile) {
        UI::TileHoverOutline(netClient.serverWorld->map);
    }
    netClient.serverWorld->DrawItems();
    netClient.serverWorld->DrawEntities();
    netClient.serverWorld->DrawParticles();
    netClient.serverWorld->DrawFlush();

    DrawRectangleRec(cameraRect, Fade(BLACK, (float)(1.0 - g_clock.daylightPerc)));

#if CL_DEMO_VIEW_RTREE
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
    thread_local std::vector<void *> matches;
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

#if CL_DEMO_AI_TRACKING
    {
        for (size_t i = 0; i < ARRAY_SIZE(world->enemies); i++) {
            const Slime &slime = world->enemies[i];
            if (slime.type) {
                Vector3 center = sprite_world_center(slime.sprite, slime.body.position, slime.sprite.scale);
                DrawCircle((int)center.x, (int)center.y, METERS_TO_PIXELS(10.0f), Fade(ORANGE, 0.3f));
            }
        }
    }
#endif

    UI::Dialog(*netClient.serverWorld);

    EndMode2D();
}

void GameClient::PlayMode_DrawScreen(double frameDt, PlayerControllerState &input)
{
    assert(netClient.serverWorld);

    // Render mouse tile tooltip
    if (input.dbgFindMouseTile) {
        UI::TileHoverTip(g_fonts.fontSmall, netClient.serverWorld->map);
    }

    UI::Minimap(g_fonts.fontSmall, *netClient.serverWorld);

    // TODO(cleanup): Noise test
    //DrawTexturePro(noise,
    //    { 0, 0, (float)noise.width, (float)noise.height },
    //    { screenSize.x - 4 - 256, (float)4 + world->map->minimap.height + 4, 256, 256 },
    //    { 0, 0 }, 0, WHITE);

    UI::Chat(g_fonts.fontSdf72, 16, *netClient.serverWorld, netClient, input.escape);

    // Render HUD
    UI::DebugStats debugStats{};
#if SHOW_DEBUG_STATS
    debugStats.tick = netClient.serverWorld->tick;
    debugStats.frameDt = frameDt;
    debugStats.cameraSpeed = g_spycam.cameraSpeed;
    debugStats.tilesDrawn = tilesDrawn;
    debugStats.effectsActive = netClient.serverWorld->particleSystem.EffectsActive();
    debugStats.particlesActive = netClient.serverWorld->particleSystem.ParticlesActive();
#endif
    if (netClient.server) {
        debugStats.rtt = enet_peer_get_rtt(netClient.server);
        debugStats.packets_sent = enet_peer_get_packets_sent(netClient.server);
        debugStats.packets_lost = enet_peer_get_packets_lost(netClient.server);
        debugStats.bytes_sent = enet_peer_get_bytes_sent(netClient.server);
        debugStats.bytes_recv = enet_peer_get_bytes_received(netClient.server);
        debugStats.cl_input_seq = netClient.inputSeq;
        if (netClient.worldHistory.Count()) {
            const WorldSnapshot &worldSnapshot = netClient.worldHistory.Last();
            debugStats.sv_input_ack = worldSnapshot.lastInputAck;
            debugStats.input_buf_size = debugStats.cl_input_seq - debugStats.sv_input_ack;
        }
    }
    UI::HUD(g_fonts.fontSmall, *netClient.serverWorld, debugStats);
    //UI::QuickHUD(fontSdf24, player, *world->map);
    Player *player = netClient.serverWorld->FindPlayer(netClient.serverWorld->playerId);
    if (player) {
        UI::Inventory(g_item_catalog.Tex(), *player, netClient, input.escape, inventoryActive);
    }

    UI::Menubar(netClient);
    UI::ShowDemoWindow();
    UI::Netstat(netClient, renderAt);
    UI::ItemProtoEditor(*netClient.serverWorld);
    UI::ParticleConfig(*netClient.serverWorld);
    UI::InGameMenu(input.escape, netClient.ConnectedAndSpawned());
}

ErrorType GameClient::Run(void)
{
    error_init("game.log");
    Init();

    while (!WindowShouldClose() && !UI::QuitRequested()) {
        // Time is of the essence
        const double frameDt = g_clock.update(glfwGetTime());

        E_ASSERT(PlayMode_Network(), "Failed to do message processing");

        PlayMode_Audio(frameDt);

        PlayerControllerState input{};
        bool connected = netClient.ConnectedAndSpawned();
        if (connected) {
            PlayMode_PollController(input);
            PlayMode_HandleInput(input);
            PlayMode_Update(frameDt, input);
            PlayMode_UpdateCamera(frameDt, input);
        }
        UI::Update(input, screenSize, &g_spycam);

        // Render prepare
        BeginDrawing();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render background
        DrawTexture(checkboardTexture, 0, 0, WHITE);
        rlDrawRenderBatchActive();

        // Render game
        if (connected) {
            PlayMode_DrawWorld(input);
            PlayMode_DrawScreen(frameDt, input);
        } else {
            bool escape = IsKeyPressed(KEY_ESCAPE);
            UI::MainMenu(escape, *this);
        }

        // Render flip
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        EndDrawing();
    }

    // Cleanup
    // TODO: Wrap these in classes to use destructors?
    UnloadShader(g_sdfShader);
    UnloadFont(g_fonts.fontSdf24);
    UnloadFont(g_fonts.fontSdf72);
#if CL_PIXEL_FIXER
    UnloadShader(pixelFixer);
#endif
    UnloadTexture(checkboardTexture);
    UnloadFont(g_fonts.fontSmall);
    //UnloadFont(fontBig);
    Catalog::g_tracks.Unload();
    Catalog::g_sounds.Unload();

    // TODO: Fix exception in miniaudio when calling this
    CloseAudioDevice();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    error_free();
    return ErrorType::Success;
}
