#include "catalog/items.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "catalog/tracks.h"
#include "draw_command.h"
#include "catalog/fonts.h"
#include "fx/fx.h"
#include "game_client.h"
#include "healthbar.h"
#include "input_mode.h"
#include "loot_table.h"
#include "net_client.h"
#include "particles.h"
#include "rtree.h"
#include "spycam.h"
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

#include <atomic>

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

void GameClient::Init(void)
{
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);  // Disable default Escape exit key, we'll handle escape ourselves
    screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    const char *fontName = "C:/Windows/Fonts/consolab.ttf";
    //const char *fontName = "resources/Hack-Bold.ttf";
    //const char *fontName = "resources/PressStart2P-vaV7.ttf";

    g_fonts.imFontHack16 = io.Fonts->AddFontFromFileTTF(fontName, 16.0f);
    g_fonts.imFontHack32 = io.Fonts->AddFontFromFileTTF(fontName, 32.0f);
    g_fonts.imFontHack48 = io.Fonts->AddFontFromFileTTF(fontName, 48.0f);
    g_fonts.imFontHack64 = io.Fonts->AddFontFromFileTTF(fontName, 64.0f);
    io.FontDefault = g_fonts.imFontHack16;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    GLFWwindow *glfwWindow = glfwGetCurrentContext();
    assert(glfwWindow);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    g_fonts.fontSmall = LoadFontEx(fontName, 16, 0, 0);
    //Font fontBig = LoadFontEx(fontName, 72, 0, 0);
    assert(g_fonts.fontSmall.texture.id);
    //assert(fontBig.texture.id);
    GuiSetFont(g_fonts.fontSmall);
    //HealthBar::SetFont(GetFontDefault());
    HealthBar::SetFont(g_fonts.fontSmall);

#if CL_PIXEL_FIXER
    pixelFixer                     = LoadShader("resources/pixelfixer.vs", "resources/pixelfixer.fs");
    pixelFixerScreenSizeUniformLoc = GetShaderLocation(pixelFixer, "screenSize");
    SetShaderValue(pixelFixer, pixelFixerScreenSizeUniformLoc, &screenSize, SHADER_UNIFORM_VEC2);
#endif

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
        g_sdfShader = LoadShader(0, "resources/sdf.fs");
        SetTextureFilter(g_fonts.fontSdf72.texture, TEXTURE_FILTER_BILINEAR);  // Required for SDF font
    }

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);

    spycam.Init({ screenSize.x * 0.5f, screenSize.y * 0.5f });

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("ERROR: Failed to initialized audio device\n");
    }

    Catalog::g_items.LoadTextures();
    Catalog::g_items.LoadData();
    Catalog::g_particleFx.Load();
    Catalog::g_sounds.Load();
    Catalog::g_spritesheets.Load();
    Catalog::g_tracks.Load();
    tileset_init();

    Catalog::g_mixer.masterVolume = 0.75f;
    Catalog::g_mixer.musicVolume = 0.3f;
    Catalog::g_sounds.mixer.volumeLimit[(size_t)Catalog::SoundID::GemBounce] = 0.8f;
    Catalog::g_sounds.mixer.volumeLimit[(size_t)Catalog::SoundID::Whoosh] = 0.6f;

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

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

    //const int targetFPS = 60;
    //SetTargetFPS(targetFPS);
    SetWindowState(FLAG_VSYNC_HINT);
}

void GameClient::PlayMode_PollController(PlayerControllerState &input)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        inputMode = INPUT_MODE_IMGUI;
    } else if (chatVisible) {
        inputMode = INPUT_MODE_CHAT;
    } else {
        inputMode = INPUT_MODE_PLAY;
    }

    //if (inputMode == INPUT_MODE_MENU) {
    //    io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NavNoCaptureKeyboard;
    //} else {
    //    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse & ~ImGuiConfigFlags_NavNoCaptureKeyboard;
    //}

    const bool processMouse = inputMode == INPUT_MODE_PLAY && !io.WantCaptureMouse;
    const bool processKeyboard = inputMode == INPUT_MODE_PLAY;
    input.Query(processMouse, processKeyboard, spycam.freeRoam);
}

ErrorType GameClient::PlayMode_Network(double frameDt, PlayerControllerState &input)
{
    E_ASSERT(netClient.Receive(), "Failed to receive packets");

    if (UI::DisconnectRequested(netClient.IsDisconnected())) {
        netClient.Disconnect();
        if (localServer) {
            args.serverQuit = true;
            delete localServer;
            localServer = 0;
            args.serverQuit = false;
        }
        world = nullptr;
        tickAccum = 0;
        sendInputAccum = 0;
        return ErrorType::Success;
    }

    bool connected = netClient.ConnectedAndSpawned();
    if (connected && world != netClient.serverWorld) {
        // We joined a server, take on the server's world
        world = netClient.serverWorld;
        tickAccum = 0;
        sendInputAccum = 0;
    }

    return ErrorType::Success;
}

void GameClient::PlayMode_Audio(double frameDt, PlayerControllerState &input)
{
    SetMasterVolume(VolumeCorrection(Catalog::g_mixer.masterVolume));
    Catalog::g_tracks.Update((float)frameDt);
}

void GameClient::PlayMode_HandleInput(double frameDt, PlayerControllerState &input)
{
    UI::HandleInput(input);

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
        spycam.Reset();
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
        //ParticleEffectParams rainbowParams{};
        //rainbowParams.durationMin = 3.0f;
        //rainbowParams.durationMax = rainbowParams.durationMin;
        //rainbowParams.particleCountMin = 256;
        //rainbowParams.particleCountMax = rainbowParams.particleCountMin;
        //ParticleEffect *rainbowFx = world->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Rainbow, player.body.WorldPosition(), rainbowParams);
        //if (rainbowFx) {
        //    rainbowFx->particleCallbacks[(size_t)ParticleEffect_ParticleEvent::AfterUpdate] = { RainbowParticlesDamagePlayer, &player };
        //}
        //Catalog::g_sounds.Play(Catalog::SoundID::RainbowSparkles, 1.0f);
        //world->chatHistory.PushDebug(CSTR("You pressed the send random chat message button. Congrats."));
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
    Player *player = world->FindPlayer(world->playerId);

    // TODO(cleanup): jump
    if (player->body.OnGround() && input.dbgJump) {
        player->body.ApplyForce({ 0, 0, METERS_TO_PIXELS(4.0f) });
    }

    renderAt = 0;
#if 0
    while (tickAccum > tickDt) {
#endif
        if (netClient.worldHistory.Count()) {
            const WorldSnapshot &worldSnapshot = netClient.worldHistory.Last();
            //printf("worldTick: %u, lastSnapshot: %u, delta: %u\n", world->tick, worldSnapshot.tick, worldSnapshot.tick - world->tick);
            world->tick = worldSnapshot.tick;

            // Update world state from worldSnapshot and re-apply input with input.tick > snapshot.tick
            netClient.ReconcilePlayer();

            //while (tickAccum > tickDt) {
            netClient.inputSeq = MAX(1, netClient.inputSeq + 1);
            InputSample &inputSample = netClient.inputHistory.Alloc();
            inputSample.FromController(player->id, netClient.inputSeq, frameDt, input);

            assert(world->map);
            player->Update(inputSample, *world->map);
            world->particleSystem.Update(frameDt);
            world->itemSystem.Update(frameDt);

            //    tickAccum -= tickDt;
            //}

            // TODO: Roll-up inputs when v-sync disabled
            // Send input to server at a fixed rate that matches server tick rate
            while (sendInputAccum > sendInputDt) {
                netClient.SendPlayerInput();
                sendInputAccum -= sendInputDt;
            }

            world->CL_DespawnStaleEntities();

            // Interpolate all of the other entities in the world
            //printf("RTT: %u RTTv: %u LRT: %u LRTv: %u HRTv %u\n",
            //    netClient.server->roundTripTime,
            //    netClient.server->roundTripTimeVariance,
            //    netClient.server->lastRoundTripTime,
            //    netClient.server->lastRoundTripTimeVariance,
            //    netClient.server->highestRoundTripTimeVariance
            //);
            //renderAt = now - (1.0 / SNAPSHOT_SEND_RATE) - (1.0 / (netClient.server->lastRoundTripTime + netClient.server->lastRoundTripTimeVariance));
            renderAt = frameStart - (1.0 / (SNAPSHOT_SEND_RATE * 1.5));
            world->CL_Interpolate(renderAt);
            //world->CL_Extrapolate(now - renderAt);
        }
#if 0
        tickAccum -= tickDt;
    }
#endif
}

void GameClient::PlayMode_UpdateCamera(double frameDt, PlayerControllerState &input)
{
    if (!spycam.freeRoam) {
        Player *player = world->FindPlayer(world->playerId);
        assert(player);
        spycam.cameraGoal = player->body.GroundPosition();
    }
    spycam.Update(input, frameDt);
}

void GameClient::PlayMode_DrawWorld(double frameDt, PlayerControllerState &input)
{
    Camera2D cam = spycam.GetCamera();
    BeginMode2D(cam);

    Rectangle cameraRect = spycam.GetRect();
    world->EnableCulling(cameraRect);

#if CL_PIXEL_FIXER
    BeginShaderMode(pixelFixer);
#endif
    tilesDrawn = world->DrawMap(spycam);
#if CL_PIXEL_FIXER
    EndShaderMode();
#endif

    if (input.dbgFindMouseTile) {
        UI::TileHoverOutline(*world->map);
    }
    world->DrawItems();
    world->DrawEntities();
    world->DrawParticles();
    world->DrawFlush();

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
}

void GameClient::PlayMode_DrawScreen(double frameDt, PlayerControllerState &input)
{
    assert(world);

    // Render mouse tile tooltip
    if (input.dbgFindMouseTile) {
        UI::TileHoverTip(g_fonts.fontSmall, *world->map);
    }

    UI::Minimap(g_fonts.fontSmall, *world);

    // TODO(cleanup): Noise test
    //DrawTexturePro(noise,
    //    { 0, 0, (float)noise.width, (float)noise.height },
    //    { screenSize.x - 4 - 256, (float)4 + world->map->minimap.height + 4, 256, 256 },
    //    { 0, 0 }, 0, WHITE);

    UI::Chat(g_fonts.fontSdf72, 16, *world, netClient, inputMode == INPUT_MODE_PLAY || inputMode == INPUT_MODE_CHAT, chatVisible, input.escape);

    // Render HUD
    UI::DebugStats debugStats{};
#if SHOW_DEBUG_STATS
    debugStats.tick = world->tick;
    debugStats.tickAccum = tickAccum;
    debugStats.frameDt = frameDt;
    debugStats.cameraSpeed = spycam.cameraSpeed;
    debugStats.tilesDrawn = tilesDrawn;
    debugStats.effectsActive = world->particleSystem.EffectsActive();
    debugStats.particlesActive = world->particleSystem.ParticlesActive();
#endif
    if (netClient.server) {
        debugStats.rtt = enet_peer_get_rtt(netClient.server);
        debugStats.packets_sent = enet_peer_get_packets_sent(netClient.server);
        debugStats.packets_lost = enet_peer_get_packets_lost(netClient.server);
        debugStats.bytes_sent = enet_peer_get_bytes_sent(netClient.server);
        debugStats.bytes_recv = enet_peer_get_bytes_received(netClient.server);
    }
    UI::HUD(g_fonts.fontSmall, *world, debugStats);
    //UI::QuickHUD(fontSdf24, player, *world->map);
    if (inventoryActive) {
        Player *player = world->FindPlayer(world->playerId);
        if (player) {
            UI::Inventory(Catalog::g_items.Tex(), *player, netClient, input.escape, inventoryActive);
        }
    }

    rlDrawRenderBatchActive();

    UI::Menubar(netClient);
    UI::ShowDemoWindow();
    UI::Netstat(netClient, renderAt);
    UI::ParticleConfig(*world);
    UI::InGameMenu(input.escape, netClient.ConnectedAndSpawned());
}

ErrorType GameClient::Run(void)
{
    Init();

    frameStart = glfwGetTime();
    while (!WindowShouldClose() && !UI::QuitRequested()) {
        // Time is of the essence
        double now = glfwGetTime();
        double frameDt = now - frameStart;
        // Limit delta time to prevent spiraling for after long hitches (e.g. hitting a breakpoint)
        tickAccum += MIN(frameDt, tickDtMax);
        sendInputAccum += MIN(sendInputDt, sendInputDtMax);
        frameStart = now;

        // Collect input
        PlayerControllerState input{};
        PlayMode_PollController(input);

        // Networking
        E_ASSERT(PlayMode_Network(frameDt, input), "Failed to do message processing");

        if (world) {
            PlayMode_Audio(frameDt, input);
            PlayMode_HandleInput(frameDt, input);
            PlayMode_Update(frameDt, input);
            PlayMode_UpdateCamera(frameDt, input);
        }

        // Render: Prologue
        BeginDrawing();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        DrawTexture(checkboardTexture, 0, 0, WHITE);

        rlDrawRenderBatchActive();
        UI::Begin(screenSize, &spycam);

        // Render: Kernel
        if (world) {
            PlayMode_DrawWorld(frameDt, input);
            PlayMode_DrawScreen(frameDt, input);
        } else {
            bool escape = IsKeyPressed(KEY_ESCAPE);
            UI::MainMenu(escape, *this);
        }

        // Render: Epilogue
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

    return ErrorType::Success;
}
