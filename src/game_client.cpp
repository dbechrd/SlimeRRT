#include "catalog/items.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "catalog/tracks.h"
#include "draw_command.h"
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

const char *GameClient::LOG_SRC = "GameClient";

ErrorType GameClient::Run(void)
{
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);  // Disable default Escape exit key, we'll handle escape ourselves
    Vector2 screenSize{ (float)GetRenderWidth(), (float)GetRenderHeight() };

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

    if (netClient.Connect(args.host, args.port, args.user, args.pass) != ErrorType::Success) {
        TraceLog(LOG_ERROR, "Failed to connect to local server");
    }

    const char *fontName = "C:/Windows/Fonts/consola.ttf";
    //const char *fontName = "resources/UbuntuMono-Regular.ttf";
    Font fontSmall = LoadFontEx(fontName, 14, 0, 0);
    //Font fontBig = LoadFontEx(fontName, 72, 0, 0);
    assert(fontSmall.texture.id);
    //assert(fontBig.texture.id);
    GuiSetFont(fontSmall);
    HealthBar::SetFont(GetFontDefault());

#if PIXEL_FIXER
    Shader pixelFixer = LoadShader("resources/pixelfixer.vs", "resources/pixelfixer.fs");
    const int pixelFixerScreenSizeUniformLoc = GetShaderLocation(pixelFixer, "screenSize");
    SetShaderValue(pixelFixer, pixelFixerScreenSizeUniformLoc, &screenSize, SHADER_UNIFORM_VEC2);
#endif

    // SDF font generation from TTF font
    Font fontSdf24{};
    Font fontSdf72{};
    {
        unsigned int fileSize = 0;
        unsigned char *fileData = 0;
        Image atlas{};

        fontSdf24.baseSize = 24;
        fontSdf24.glyphCount = 95;
        // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 0 (defaults to 95)
        // Loading file to memory
        fileData = LoadFileData(fontName, &fileSize);
        fontSdf24.glyphs = LoadFontData(fileData, fileSize, fontSdf24.baseSize, 0, 0, FONT_SDF);
        // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 0 px, pack method: 1 (Skyline algorythm)
        atlas = GenImageFontAtlas(fontSdf24.glyphs, &fontSdf24.recs, 95, fontSdf24.baseSize, 0, 1);
        fontSdf24.texture = LoadTextureFromImage(atlas);
        UnloadImage(atlas);
        UnloadFileData(fileData);  // Free memory from loaded file

        fontSdf72.baseSize = 72;
        fontSdf72.glyphCount = 95;
        // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 0 (defaults to 95)
        // Loading file to memory
        fileData = LoadFileData(fontName, &fileSize);
        fontSdf72.glyphs = LoadFontData(fileData, fileSize, fontSdf72.baseSize, 0, 0, FONT_SDF);
        // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 0 px, pack method: 1 (Skyline algorythm)
        atlas = GenImageFontAtlas(fontSdf72.glyphs, &fontSdf72.recs, 95, fontSdf72.baseSize, 0, 1);
        fontSdf72.texture = LoadTextureFromImage(atlas);
        UnloadImage(atlas);
        UnloadFileData(fileData);  // Free memory from loaded file

        // Load SDF required shader (we use default vertex shader)
        g_sdfShader = LoadShader(0, "resources/sdf.fs");
        SetTextureFilter(fontSdf72.texture, TEXTURE_FILTER_BILINEAR);  // Required for SDF font
    }

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);

    Spycam spycam{};
    spycam.Init({ screenSize.x * 0.5f, screenSize.y * 0.5f });

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("ERROR: Failed to initialized audio device\n");
    }

    Catalog::g_items.Load();
    Catalog::g_mixer.Load();
    Catalog::g_particleFx.Load();
    Catalog::g_sounds.Load();
    Catalog::g_spritesheets.Load();
    Catalog::g_tracks.Load();
    tileset_init();

    Catalog::g_mixer.masterVolume = 0.0f;
    Catalog::g_mixer.musicVolume = 0.0f;
    Catalog::g_sounds.mixer.volumeLimit[(size_t)Catalog::SoundID::GemBounce] = 0.8f;

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    World *lobby = new World;
    lobby->tick = 1;
    lobby->map = lobby->mapSystem.Generate(lobby->rtt_rand, 128, 128);
    Slime &sam = lobby->SpawnSam();
    world = lobby;

    {
        Player *player = lobby->AddPlayer(0);
        assert(player);
        lobby->playerId = player->id;
        memcpy(player->name, CSTR("dandy"));
        player->nameLength = (uint32_t)strlen(player->name);
        player->combat.hitPoints = MAX(0, player->combat.hitPointsMax - 25);
        //player->body.position.x = 1373.49854f;

        // 1600 x 900
        //player->body.position.x = 1373.498f;

        // 1610 x 910
        //player->body.position.x = 1457.83557f;
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
    bool chatVisible = false;
    bool menuActive = false;
    bool mixerActive = false;
    bool disconnectRequested = false;
    bool quit = false;

    InputMode inputMode = INPUT_MODE_PLAY;

    bool show_demo_window = false;
    //---------------------------------------------------------------------------------------

#define KEY_DOWN(key)

    // Main game loop
    while (!quit && !WindowShouldClose()) {
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
        // Input
        //----------------------------------------------------------------------
        if (menuActive) {
            inputMode = INPUT_MODE_MENU;
        } else if (io.WantCaptureKeyboard) {
            inputMode = INPUT_MODE_IMGUI;
        } else if (chatVisible) {
            inputMode = INPUT_MODE_CHAT;
        } else {
            inputMode = INPUT_MODE_PLAY;
        }

        if (inputMode == INPUT_MODE_MENU) {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NavNoCaptureKeyboard;
        } else {
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse & ~ImGuiConfigFlags_NavNoCaptureKeyboard;
        }

        const bool processMouse = inputMode == INPUT_MODE_PLAY && !io.WantCaptureMouse;
        const bool processKeyboard = inputMode == INPUT_MODE_PLAY;
        PlayerControllerState input = PlayerControllerState::Query(processMouse, processKeyboard, spycam.freeRoam);
        bool escape = IsKeyPressed(KEY_ESCAPE);

        // HACK: No way to check if Raylib is currently recording.. :(
        //if (gifRecording) {
        //    tickDt = 1.0 / 10.0;
        //}

        //----------------------------------------------------------------------
        // Networking
        //----------------------------------------------------------------------
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
            if (netClient.serverWorld) {
                delete netClient.serverWorld;
                netClient.serverWorld = nullptr;
            }
            world = lobby;
            tickAccum = 0;
            sendInputAccum = 0;
        }

        //----------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------
        Player *playerPtr = world->FindPlayer(world->playerId);
        assert(playerPtr);
        Player &player = *playerPtr;

        if (player.body.OnGround() && IsKeyPressed(KEY_SPACE)) {
            // jump
            player.body.ApplyForce({ 0, 0, METERS_TO_PIXELS(4.0f) });
        }

        double renderAt = 0;
        if (connectedToServer) {
            if (netClient.worldHistory.Count()) {
                const WorldSnapshot &worldSnapshot = netClient.worldHistory.Last();

                static uint32_t lastTickAck = 0;
                if (lastTickAck < worldSnapshot.tick) {
                    lastTickAck = worldSnapshot.tick;

                    world->tick = worldSnapshot.tick;

                    // TODO: Update world state from worldSnapshot and re-apply input with input.tick > snapshot.tick
                    //netClient.ReconcilePlayer(tickDt);
                }
                // TODO: Update world state from worldSnapshot and re-apply input with input.tick > snapshot.tick
                netClient.ReconcilePlayer(tickDt);

                //while (tickAccum > tickDt) {
                    InputSample &inputSample = netClient.inputHistory.Alloc();
                    inputSample.FromController(player.id, lastTickAck, input);

                    assert(world->map);
                    player.Update(tickDt, inputSample, *world->map);
                    world->particleSystem.Update(tickDt);
                    world->itemSystem.Update(tickDt);

                //    tickAccum -= tickDt;
                //}

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
                renderAt = now - (1.0 / (SNAPSHOT_SEND_RATE * 1.5));
                world->CL_Interpolate(renderAt);
                //world->CL_Extrapolate(now - renderAt);
            }
        } else {
#if 1
            while (tickAccum > tickDt) {
                InputSample inputSample{};
                inputSample.FromController(player.id, world->tick, input);

                assert(world->map);
                player.Update(tickDt, inputSample, *world->map);

                world->SV_Simulate(tickDt);
                world->particleSystem.Update(tickDt);
                world->itemSystem.Update(tickDt);
                world->CL_DespawnStaleEntities();


                //WorldSnapshot &worldSnapshot = netClient.worldHistory.Alloc();
                //assert(!worldSnapshot.tick);  // ringbuffer alloc fucked up and didn't zero slot
                //worldSnapshot.playerId = player.id;
                ////worldSnapshot.lastInputAck = world->tick;
                //worldSnapshot.tick = world->tick;
                //world->GenerateSnapshot(worldSnapshot);

                world->tick++;
                tickAccum -= tickDt;
            }

            //// Interpolate all of the other entities in the world
            //double renderAt = glfwGetTime() - tickDt * 2;
            //world->Interpolate(renderAt);
#endif
        }

        static bool samTreasureRoom = false;
        if (!samTreasureRoom && world == lobby && sam.combat.diedAt) {
            const Vector2 playerBC = player.body.GroundPosition();
            uint32_t chestX = MAX(0, int(playerBC.x / TILE_W));
            uint32_t chestY = MAX(0, int(playerBC.y / TILE_W) - 2);
            Structure::Spawn(*world->map, chestX - 3, chestY - 4);
            Vector3 chestPos{ (chestX * TILE_W) + TILE_W * 0.5f, float(chestY * TILE_W), 0.0f };

            // TODO: Make chest/gems items.. or?
            // - D2: flying item animation -> ground item
            // - Terraria: Immediately items with physics that gravitate toward player
            // - Minecraft: Immediately items with physics, lots of friction on ground, gravity scoop
            // - What about DeathSpank?
            //world->particleSystem.GenerateEffect(Catalog::ParticleEffectID::GoldenChest, 1, chestPos, 4.0f);
            //world->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Gem, 8, chestPos, 3.0f);
            //Catalog::g_sounds.Play(Catalog::SoundID::Gold, 1.0f + dlb_rand32f_variance(0.4f));
            samTreasureRoom = true;
        }

        //----------------------------------------------------------------------
        // Input handling
        //----------------------------------------------------------------------
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

        if (IsWindowResized() || input.toggleFullscreen) {
            if (IsWindowResized()) {
                screenSize.x = (float)GetRenderWidth() - GetRenderWidth() % 2;
                screenSize.y = (float)GetRenderHeight() - GetRenderHeight() % 2;
                SetWindowSize((int)screenSize.x, (int)screenSize.y);
            }
            if (input.toggleFullscreen) {
                static Vector2 restoreSize = screenSize;
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
#if PIXEL_FIXER
            SetShaderValue(pixelFixer, pixelFixerScreenSizeUniformLoc, &screenSize, SHADER_UNIFORM_VEC2);
#endif
            spycam.Reset();
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

        //----------------------------------------------------------------------
        // Camera
        //----------------------------------------------------------------------
        if (!spycam.freeRoam) {
            spycam.cameraGoal = player.body.GroundPosition();
        }
        spycam.Update(input, frameDt);
        Rectangle cameraRect = spycam.GetRect();

        const Vector2 mousePosScreen = GetMousePosition();
        Vector2 mousePosWorld{};
        mousePosWorld.x += cameraRect.x + mousePosScreen.x * spycam.GetInvZoom();
        mousePosWorld.y += cameraRect.y + mousePosScreen.y * spycam.GetInvZoom();

        //----------------------------------------------------------------------
        // Render world
        //----------------------------------------------------------------------
        BeginDrawing();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        UI::Begin(mousePosScreen, mousePosWorld, screenSize, &spycam);

        ClearBackground(ORANGE);
        DrawTexture(checkboardTexture, 0, 0, WHITE);

        Camera2D cam = spycam.GetCamera();
        BeginMode2D(cam);

        world->EnableCulling(cameraRect);

#if PIXEL_FIXER
        BeginShaderMode(pixelFixer);
#endif
        size_t tilesDrawn = world->DrawMap(spycam.GetZoomMipLevel());
#if PIXEL_FIXER
        EndShaderMode();
#endif

        if (input.dbgFindMouseTile) {
            UI::TileHoverOutline(*world->map);
        }
        world->DrawItems();
        world->DrawEntities();
        world->DrawParticles();
        world->DrawFlush();

        Vector2 playerPos = player.body.GroundPosition();
        const Tile *playerTileLeft = world->map->TileAtWorld(playerPos.x - 15.0f, playerPos.y);
        const Tile *playerTileRight = world->map->TileAtWorld(playerPos.x + 15.0f, playerPos.y);
        if (playerTileLeft && playerTileLeft->tileType == TileType::Water &&
            playerTileRight && playerTileRight->tileType == TileType::Water)
        {
            auto tilesetId = world->map->tilesetId;
            Tileset &tileset = g_tilesets[(size_t)tilesetId];
            Rectangle tileRect = tileset_tile_rect(tilesetId, TileType::Water);
            assert(tileRect.width == TILE_W);
            assert(tileRect.height == TILE_W);

            Vector3 playerGut3D = player.GetAttachPoint(Player::AttachPoint::Gut);
            Vector2 playerGut2D = { playerGut3D.x, playerGut3D.y - playerGut3D.z + 10.0f };
            float offsetX = fmodf(playerGut2D.x, TILE_W);
            float offsetY = fmodf(playerGut2D.y, TILE_W);

            Rectangle dstTopMid{
                playerGut2D.x - offsetX,
                playerGut2D.y,
                TILE_W,
                TILE_W - offsetY
            };

            Rectangle dstTopLeft = dstTopMid;
            dstTopLeft.x -= TILE_W;

            Rectangle dstTopRight = dstTopMid;
            dstTopRight.x += TILE_W;

            Rectangle srcTop = tileRect;
            srcTop.y += offsetY;
            srcTop.height -= offsetY;

            Rectangle dstBotMid{
                playerGut2D.x - offsetX,
                playerGut2D.y - offsetY + TILE_W,
                TILE_W,
                TILE_W
            };

            Rectangle dstBotLeft = dstBotMid;
            dstBotLeft.x -= TILE_W;

            Rectangle dstBotRight = dstBotMid;
            dstBotRight.x += TILE_W;

            float minXWater = FLT_MAX;
            float maxXWater = FLT_MIN;

#define CHECK_AND_DRAW(src, dst) \
            tile = world->map->TileAtWorld((dst).x, (dst).y);                                  \
            if (tile && tile->tileType == TileType::Water) {                                   \
                DrawTexturePro(tileset.texture, (src), (dst), { 0, 0 }, 0, Fade(WHITE, 0.8f)); \
                minXWater = MIN(minXWater, (dst).x);                                           \
                maxXWater = MAX(maxXWater, (dst).x + (dst).width);                             \
            }

            const Tile *tile = 0;
            CHECK_AND_DRAW(srcTop, dstTopLeft);
            CHECK_AND_DRAW(srcTop, dstTopMid);
            CHECK_AND_DRAW(srcTop, dstTopRight);
            CHECK_AND_DRAW(tileRect, dstBotLeft);
            CHECK_AND_DRAW(tileRect, dstBotMid);
            CHECK_AND_DRAW(tileRect, dstBotRight);

#undef CHECK_AND_DRAW

            const Tile *playerGutTile = world->map->TileAtWorld(playerGut2D.x, playerGut2D.y);
            if (playerGutTile && playerGutTile->tileType == TileType::Water) {
                Rectangle bubblesDstTopMid{
                    playerGut2D.x - 20.0f,
                    playerGut2D.y,
                    40.0f,
                    8.0f
                };

                Rectangle bubblesSrcTop = tileRect;
                bubblesSrcTop.y += offsetY;
                bubblesSrcTop.height -= offsetY;

                //DrawCircleV({ minXWater, playerGut2D.y }, 2.0f, PINK);
                //DrawCircleV({ maxXWater, playerGut2D.y }, 2.0f, PINK);

                const float radiusDelta = (player.moveState != Player::MoveState::Idle) ? (float)(sin(now * 8) * 3) : 0.0f;
                const float radius = 20.0f + radiusDelta;
                DrawEllipse((int)playerGut2D.x, (int)playerGut2D.y, radius, radius * 0.5f, Fade(SKYBLUE, 0.4f));
            }
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

        //----------------------------------------------------------------------
        // Render screen (HUD, UI, etc.)
        //----------------------------------------------------------------------
        UI::Minimap(fontSmall, *world);

        // Render HUD
        UI::DebugStats debugStats{};
#if SHOW_DEBUG_STATS
        debugStats.tick            = world->tick;
        debugStats.tickAccum       = tickAccum;
        debugStats.frameDt         = frameDt;
        debugStats.cameraSpeed     = spycam.cameraSpeed;
        debugStats.tilesDrawn      = tilesDrawn;
        debugStats.effectsActive   = world->particleSystem.EffectsActive();
        debugStats.particlesActive = world->particleSystem.ParticlesActive();
#endif
        if (netClient.server) {
            debugStats.rtt = enet_peer_get_rtt(netClient.server);
            debugStats.packets_sent = enet_peer_get_packets_sent(netClient.server);
            debugStats.packets_lost = enet_peer_get_packets_lost(netClient.server);
            debugStats.bytes_sent = enet_peer_get_bytes_sent(netClient.server);
            debugStats.bytes_recv = enet_peer_get_bytes_received(netClient.server);
        }
        //UI::HUD(fontSmall, player, debugStats);
        UI::QuickHUD(fontSdf24, player);
        UI::Chat(fontSdf72, 20, *world, netClient, inputMode == INPUT_MODE_PLAY || inputMode == INPUT_MODE_CHAT, chatVisible, escape);

        rlDrawRenderBatchActive();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        UI::LoginForm(netClient, io, escape);
        if (mixerActive) {
            UI::Mixer();
        }
        //UI::Netstat(netClient, renderAt);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Render mouse tile tooltip
        if (input.dbgFindMouseTile) {
            UI::TileHoverTip(fontSmall, *world->map);
        }

        //----------------------------------------------------------------------
        // Menu
        //----------------------------------------------------------------------
        if (escape) {
            menuActive = !menuActive;
        }

        if (menuActive) {
            if (connectedToServer) {
                const char *menuItems[] = { "Resume", "Audio", "Log off" };
                switch (UI::Menu(fontSdf72, escape, quit, menuItems, ARRAY_SIZE(menuItems))) {
                    case 0: {    // Resume
                        menuActive = false;
                        break;
                    } case 1: {  // Audio
                        mixerActive = !mixerActive;
                        break;
                    } case 2: {  // Log off
                        disconnectRequested = true;
                        menuActive = false;
                        break;
                    }
                }
            } else {
                const char *menuItems[] = { "Resume", "Audio", "Quit" };
                switch (UI::Menu(fontSdf72, escape, quit, menuItems, ARRAY_SIZE(menuItems))) {
                    case 0: {    // Resume
                        menuActive = false;
                        break;
                    } case 1: {  // Audio
                        mixerActive = !mixerActive;
                        break;
                    } case 2: {  // Quit
                        menuActive = false;
                        quit = true;
                        break;
                    }
                }
            }
        }

        EndDrawing();
        //----------------------------------------------------------------------------------

        if (disconnectRequested && connectedToServer) {
            netClient.Disconnect();
            world = lobby;
            disconnectRequested = false;
        }
    }

    //----------------------------------------------------------------------
    // Cleanup
    //----------------------------------------------------------------------
    netClient.CloseSocket();

    // TODO: Wrap these in classes to use destructors?
    delete lobby;
    UnloadShader(g_sdfShader);
    UnloadFont(fontSdf24);
    UnloadFont(fontSdf72);
#if PIXEL_FIXER
    UnloadShader(pixelFixer);
#endif
    UnloadTexture(checkboardTexture);
    UnloadFont(fontSmall);
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
