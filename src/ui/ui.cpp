#include "error.h"
#include "net_client.h"
#include "../spycam.h"
#include "../tilemap.h"
#include "../world.h"
#include "imgui/imgui.h"
#include "GLFW/glfw3.h"
#include "raylib/raylib.h"

Vector2 UI::mouseScreen;
Vector2 UI::mouseWorld;
Vector2 UI::screenSize;
Spycam  *UI::spycam;

bool UI::showMenubar = false;
bool UI::showDemoWindow = false;
bool UI::showLoginWindow = false;
bool UI::showParticleConfig = false;

bool UI::disconnectRequested = false;
bool UI::quitRequested = false;

void UI::HandleInput(PlayerControllerState &input)
{
    if (input.dbgImgui) {
        UI::showMenubar = !UI::showMenubar;
        UI::showDemoWindow = showMenubar && IsKeyDown(KEY_LEFT_SHIFT);
    }
}

bool UI::DisconnectRequested(bool connectedToServer)
{
    if (!connectedToServer) {
        disconnectRequested = false;
    }
    return disconnectRequested;
}

bool UI::QuitRequested()
{
    return quitRequested;
}

void UI::TileHoverOutline(const Tilemap &map)
{
    assert(spycam);

    const Tile *mouseTile = map.TileAtWorld(mouseWorld.x, mouseWorld.y);
    if (!mouseTile) {
        return;
    }

    const int mouseTileX = (int)mouseWorld.x / TILE_W;
    const int mouseTileY = (int)mouseWorld.y / TILE_W;

    // Draw red outline on hovered tile
    const int zoomMipLevel = spycam->GetZoomMipLevel();
    Rectangle mouseTileRect{
        (float)TILE_W * mouseTileX,
        (float)TILE_W * mouseTileY,
        (float)TILE_W * zoomMipLevel,
        (float)TILE_W * zoomMipLevel
    };
    DrawRectangleLinesEx(mouseTileRect, floorf(spycam->GetInvZoom()), RED);
}

void UI::WorldGrid(const Spycam &spycam)
{
    const Rectangle &camRect = spycam.GetRect();
    for (float y = 0; y <= camRect.height; y++) {
        DrawLine(
            (int)(camRect.x + 0),
            (int)(camRect.y + y * TILE_H),
            (int)(camRect.x + camRect.width * TILE_W),
            (int)(camRect.y + y * TILE_H),
            BLACK
        );
    }
    for (float x = 0; x <= camRect.width; x++) {
        DrawLine(
            (int)(camRect.x + x * TILE_W),
            (int)(camRect.y + 0),
            (int)(camRect.x + x * TILE_W),
            (int)(camRect.y + camRect.height * TILE_H),
            BLACK
        );
    }
    const float center_y = camRect.height / 2 * TILE_H;
    const float center_x = camRect.width / 2 * TILE_W;
    DrawLineEx({ 0, (float)center_y }, { camRect.width * TILE_W, center_y }, 3.0f, RED);
    DrawLineEx({ (float)center_x, 0 }, { center_x, camRect.height * TILE_H }, 3.0f, GREEN);
}

void UI::Minimap(const Font &font, const Spycam &spycam, World &world)
{
    const Rectangle &camRect = spycam.GetRect();

    // Render minimap
    const int minimapMargin = 6;
    const int minimapBorderWidth = 2;
    const int minimapX = (int)screenSize.x - minimapMargin - world.map->minimap.width - minimapBorderWidth * 2;
    const int minimapY = minimapMargin;
    const int minimapW = world.map->minimap.width + minimapBorderWidth * 2;
    const int minimapH = world.map->minimap.height + minimapBorderWidth * 2;
    const int minimapTexX = minimapX + minimapBorderWidth;
    const int minimapTexY = minimapY + minimapBorderWidth;
    DrawRectangle(minimapX, minimapY, minimapW, minimapH, { 220, 200, 135, 255 });
    DrawRectangleLinesEx({(float)minimapX, (float)minimapY, (float)minimapW, (float)minimapH}, (float)minimapBorderWidth, BLACK);

    Player *player = world.FindPlayer(world.playerId);
    if (player) {
        world.map->GenerateMinimap(player->body.GroundPosition());
        Vector2 playerPos = player->body.GroundPosition();
        const int offsetX = world.map->CalcChunkTile(playerPos.x) - CHUNK_W / 2;
        const int offsetY = world.map->CalcChunkTile(playerPos.y) - CHUNK_W / 2;
        BeginScissorMode(minimapTexX, minimapTexY, world.map->minimap.width, world.map->minimap.height);
        DrawTexture(world.map->minimap, minimapTexX - offsetX, minimapTexY - offsetY, WHITE);
        EndScissorMode();
    }

#if 0
    // TODO: Fix relative positioning of the map markers now that the map scrolls

    // Draw slimes on map
    for (size_t i = 0; i < ARRAY_SIZE(world.slimes); i++) {
        const Slime &s = world.slimes[i];
        if (s.id) {
            float x = (s.body.WorldPosition().x / camRect.width) * minimapW + minimapX;
            float y = (s.body.WorldPosition().y / camRect.height) * minimapH + minimapY;
            DrawCircle((int)x, (int)y, 2.0f, Color{ 0, 170, 80, 255 });
        }
    }

    // Draw players on map
    for (size_t i = 0; i < ARRAY_SIZE(world.players); i++) {
        const Player &p = world.players[i];
        if (p.id) {
            float x = (p.body.WorldPosition().x / camRect.width) * minimapW + minimapX;
            float y = (p.body.WorldPosition().y / camRect.height) * minimapH + minimapY;
            const Color playerColor{ 220, 90, 20, 255 };
            DrawCircle((int)x, (int)y, 2.0f, playerColor);
            const char *pName = SafeTextFormat("%.*s", p.nameLength, p.name);
            int nameWidth = MeasureText(pName, font.baseSize);
            DrawTextFont(font, pName, x - (float)(nameWidth / 2), y - font.baseSize - 4, 0, 0, font.baseSize, YELLOW);
        }
    }
#endif
}

void UI::Menubar(const NetClient &netClient)
{
    if (!showMenubar) return;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debug")) {
            if (netClient.IsConnected()) {
                ImGui::MenuItem("Log Off", 0, &disconnectRequested);
            } else {
                ImGui::MenuItem("Log On", 0, &showLoginWindow);
            }
            ImGui::MenuItem("Demo Window", 0, &showDemoWindow);
            ImGui::MenuItem("Particle Config", 0, &showParticleConfig);

            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                quitRequested = true;
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UI::ShowDemoWindow()
{
    if (!showDemoWindow) return;

    ImGui::ShowDemoWindow(&showDemoWindow);
}

void UI::CenteredText(const char *text)
{
    float windowWidth = ImGui::GetWindowSize().x;
    float textWidth = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text(text);
}

void UI::SliderFloatLeft(const char *label, float *v, float min, float max)
{
    assert(label[0] == '#');
    assert(label[1] == '#');
    ImGui::Text(label + 2);
    ImGui::SameLine();
    ImGui::SliderFloat(label, v, min, max, "%.01f");
}

void UI::SliderIntLeft(const char *label, int *v, int min, int max)
{
    assert(label[0] == '#');
    assert(label[1] == '#');
    ImGui::Text(label + 2);
    ImGui::SameLine();
    ImGui::SliderInt(label, v, min, max);
}

void UI::CenteredSliderFloatLeft(const char *label, float *v, float min, float max)
{
    assert(label[0] == '#');
    assert(label[1] == '#');
    ImGui::Text(label + 2);
    ImGui::SameLine();
    float windowWidth = ImGui::GetWindowSize().x;
    float widthAvail = ImGui::GetContentRegionAvail().x;
    float widthUsed = windowWidth - widthAvail;
    float sliderWidth = windowWidth - widthUsed * 2.0f;
    ImGui::SetCursorPosX((windowWidth - sliderWidth) * 0.5f);
    ImGui::PushItemWidth(sliderWidth);
    ImGui::SliderFloat(label, v, min, max, "%.03f");
    ImGui::PopItemWidth();
}

void UI::LoginForm(NetClient &netClient, ImGuiIO& io, bool &escape)
{
    if (netClient.IsConnected()) {
        showLoginWindow = false;
    }
    if (!showLoginWindow) return;

#if 1
    //ImGui::SetNextWindowPos(ImVec2(screenSize.x - 240 - 10, 10));
    //ImGui::SetNextWindowSize(ImVec2(240, 0));
    auto rayDarkBlue = DARKBLUE;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(rayDarkBlue.r, rayDarkBlue.g, rayDarkBlue.b, rayDarkBlue.a));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);
    ImGui::Begin("Quick Menu##mini_menu", 0,
        //ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse
    );
    ImGui::PopStyleColor(2);
    ImGui::Text("Play with friends!");
    //if (ImGui::Button("Connect to myself", ImVec2(150, 0))) {
    //    netClient.Connect(SV_SINGLEPLAYER_HOST, SV_SINGLEPLAYER_PORT, SV_SINGLEPLAYER_USER, SV_SINGLEPLAYER_PASS);
    //}
    if (ImGui::Button("Connect to DandyNet")) {
        ImGui::OpenPopup("Connect to Server##login_window");
    }
#endif

    if (ImGui::BeginPopupModal("Connect to Server##login_window", &showLoginWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char host[SV_HOSTNAME_LENGTH_MAX + 1]{ "slime.theprogrammingjunkie.com" };
        static int  port = SV_DEFAULT_PORT;
        static char username[USERNAME_LENGTH_MAX + 1]{ "guest" };
        static char password[PASSWORD_LENGTH_MAX + 1]{ "pizza" };
        static const char *message = 0;
        bool formValid = true;

        ImGui::Text("    Host:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(232);
        ImGui::InputText("##host", host, sizeof(host)); //, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("    Port:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(89);
        ImGui::InputInt("##port", &port, 1, 100); //, ImGuiInputTextFlags_ReadOnly);
        port = CLAMP(port, 0, USHRT_MAX);

        ImGui::Text("Username:");
        ImGui::SameLine();
        // https://github.com/ocornut/imgui/issues/455#issuecomment-167440172
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(232);
        ImGui::InputText("##username", username, sizeof(username));

        ImGui::Text("Password:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(232);
        ImGui::InputText("##password", password, sizeof(password), ImGuiInputTextFlags_Password);

        static double connectingIdxLastUpdate = 0;
        static size_t connectingIdx = 0;
        static bool triedConnecting = false;
        static double failedToConnectShownAt = 0;
        if (netClient.IsConnecting()) {
            const char *text[3]{
                "Attempting to connect.  ",
                "Attempting to connect.. ",
                "Attempting to connect...",
            };
            message = text[connectingIdx];
            triedConnecting = true;
            if (glfwGetTime() - connectingIdxLastUpdate > 0.2) {
                connectingIdx = (connectingIdx + 1) % ARRAY_SIZE(text);
                connectingIdxLastUpdate = glfwGetTime();
            }
        } else {
            if (triedConnecting) {
                message = "DandyNet is offline. :(";
                if (!failedToConnectShownAt) {
                    failedToConnectShownAt = glfwGetTime();
                } else if (glfwGetTime() - failedToConnectShownAt > 5.0) {
                    triedConnecting = false;
                    failedToConnectShownAt = 0;
                }
            } else {
                message = 0;
                static char buf[64]{};
                size_t usernameLen = strnlen(username, sizeof(username));
                size_t passwordLen = strnlen(password, sizeof(password));
                if (usernameLen < USERNAME_LENGTH_MIN || usernameLen > USERNAME_LENGTH_MAX) {
                    formValid = false;
                    snprintf(buf, sizeof(buf), "Username must be between %d-%d characters", USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
                    message = buf;
                } else if (passwordLen < PASSWORD_LENGTH_MIN || passwordLen > PASSWORD_LENGTH_MAX) {
                    formValid = false;
                    snprintf(buf, sizeof(buf), "Password must be between %d-%d characters", PASSWORD_LENGTH_MIN, PASSWORD_LENGTH_MAX);
                    message = buf;
                }
            }
            connectingIdx = 0;
        }

        if (message) {
            CenteredText(message);
        } else {
            ImGui::Text("");
        }

        ImGui::BeginDisabled(netClient.IsConnecting());

        ImGui::SetCursorPosX(177.0f);

        int stylesPushed = 1;
        ImGui::PushStyleColor(ImGuiCol_Button, formValid ? 0xFFBF8346 : 0xFF666666);
        if (!formValid) {
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF666666);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF666666);
            stylesPushed += 2;
        }
        bool login = ImGui::Button("Connect##login_window:connect", ImVec2(60, 0));
        ImGui::PopStyleColor(stylesPushed);
        if (formValid && (login ||
            IsKeyPressed(io.KeyMap[ImGuiKey_Enter]) ||
            IsKeyPressed(io.KeyMap[ImGuiKey_KeyPadEnter])))
        {
            netClient.Connect(host, (unsigned short)port, username, password);
        }

        ImGui::SameLine();
        //ImGui::PushStyleColor(ImGuiCol_Button, 0xFF999999);
        bool cancel = ImGui::Button("Cancel##login_window:cancel", ImVec2(60, 0));
        //ImGui::PopStyleColor();

        ImGui::EndDisabled();

        if (cancel || escape) {
            memset(username, 0, sizeof(username));
            memset(password, 0, sizeof(password));
            ImGui::CloseCurrentPopup();
            escape = false;
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}

void UI::Mixer(void)
{
    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(6, 398), ImGuiCond_FirstUseEver);
    auto rayDarkBlue = DARKGRAY;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(rayDarkBlue.r, rayDarkBlue.g, rayDarkBlue.b, rayDarkBlue.a));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);
    ImGui::Begin("Mixer##mixer", 0,
        0
        //ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse
    );
    ImGui::PopStyleColor(2);
    ImGui::SliderFloat("Master", &Catalog::g_mixer.masterVolume, 0.0f, 1.0f, "%.03f");
    ImGui::SliderFloat("Music ", &Catalog::g_mixer.musicVolume, 0.0f, 1.0f, "%.03f");
    ImGui::SliderFloat("Sfx   ", &Catalog::g_mixer.sfxVolume, 0.0f, 1.0f, "%.03f");
    ImGui::Text("Tracks:");
    for (size_t i = 1; i < (size_t)Catalog::TrackID::Count; i++) {
        if (ImGui::TreeNode(Catalog::TrackIDString((Catalog::TrackID)i))) {
            ImGui::SliderFloat("vLimit  ", &Catalog::g_tracks.mixer.volumeLimit[i],  0.0f, 1.0f, "%.03f");
            ImGui::SliderFloat("vVolume ", &Catalog::g_tracks.mixer.volume[i],       0.0f, 1.0f, "%.03f");
            ImGui::SliderFloat("vSpeed  ", &Catalog::g_tracks.mixer.volumeSpeed[i],  0.0f, 2.0f, "%.03f");
            ImGui::SliderFloat("vTarget ", &Catalog::g_tracks.mixer.volumeTarget[i], 0.0f, 1.0f, "%.03f");
            ImGui::TreePop();
        }
    }
    ImGui::Text("Sfx:");
    for (size_t i = 1; i < (size_t)Catalog::SoundID::Count; i++) {
        if (ImGui::TreeNode(Catalog::SoundIDString((Catalog::SoundID)i))) {
            ImGui::SliderFloat("vLimit  ", &Catalog::g_sounds.mixer.volumeLimit[i], 0.0f, 1.0f, "%.03f");
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void UI::ParticleConfig(World &world, Player &player)
{
    static ParticleEffectParams params{};

    if (!showParticleConfig) {
        return;
    }

    const ImVec2 inventorySize{ 540.0, 500.0f };
    const float pad = 80.0f;
    const float left = screenSize.x - pad - inventorySize.x;
    const float top = pad;
    ImGui::SetNextWindowPos(ImVec2(left, top), ImGuiCond_Once);
    ImGui::SetNextWindowSize(inventorySize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

    auto bgWindow = BLACK;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(bgWindow.r, bgWindow.g, bgWindow.b, 0.7f * 255.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);

    ImGui::Begin("##particle_config", &showParticleConfig,
        //ImGuiWindowFlags_NoDecoration |
        //ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    static bool ignoreEmpty = false;
    ImGui::Text("Particle Effect");
    SliderIntLeft(  "##particleCountMin ", &params.particleCountMin, 1, 256);
    SliderIntLeft(  "##particleCountMax ", &params.particleCountMax, 1, 256);
    SliderFloatLeft("##durationMin      ", &params.durationMin, 0.1f, 10.0f);
    SliderFloatLeft("##durationMax      ", &params.durationMax, 0.1f, 10.0f);
    SliderFloatLeft("##spawnDelayMin    ", &params.spawnDelayMin, 0.0f, 10.0f);
    SliderFloatLeft("##spawnDelayMax    ", &params.spawnDelayMax, 0.0f, 10.0f);
    SliderFloatLeft("##lifespanMin      ", &params.lifespanMin, 0.1f, 10.0f);
    SliderFloatLeft("##lifespanMax      ", &params.lifespanMax, 0.1f, 10.0f);
    SliderFloatLeft("##spawnScaleFirst  ", &params.spawnScaleFirst, 0.1f, 10.0f);
    SliderFloatLeft("##spawnScaleLast   ", &params.spawnScaleLast, 0.1f, 10.0f);
    SliderFloatLeft("##velocityXMin     ", &params.velocityXMin, -10.0f, 10.0f);
    SliderFloatLeft("##velocityXMax     ", &params.velocityXMax, -10.0f, 10.0f);
    SliderFloatLeft("##velocityYMin     ", &params.velocityYMin, -10.0f, 10.0f);
    SliderFloatLeft("##velocityYMax     ", &params.velocityYMax, -10.0f, 10.0f);
    SliderFloatLeft("##velocityZMin     ", &params.velocityZMin, -10.0f, 10.0f);
    SliderFloatLeft("##velocityZMax     ", &params.velocityZMax, -10.0f, 10.0f);
    SliderFloatLeft("##friction         ", &params.friction, 0.0f, 10.0f);
    if (ImGui::Button("Generate")) {
        ParticleEffect *blood = world.particleSystem.GenerateEffect(
            Catalog::ParticleEffectID::Blood,
            v3_add(player.body.WorldPosition(), { 0, 0, 40 }),
            params);
        if (blood) {
            blood->effectCallbacks[(size_t)ParticleEffect_Event::BeforeUpdate] = { ParticlesFollowPlayerGut, &player };
        }
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::End();
}

void UI::Netstat(NetClient &netClient, double renderAt)
{
    ImGui::SetNextWindowSize(ImVec2(380, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(360, 100), ImGuiCond_FirstUseEver);
    auto rayDarkBlue = DARKGRAY;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(rayDarkBlue.r, rayDarkBlue.g, rayDarkBlue.b, rayDarkBlue.a));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);
    ImGui::Begin("Network##netstat", 0,
        0
        //ImGuiWindowFlags_NoTitleBar
        //ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse
    );
    ImGui::PopStyleColor(2);

    const size_t snapshotCount = netClient.worldHistory.Count();
    if (snapshotCount) {
        assert(snapshotCount <= CL_WORLD_HISTORY);
        float times[CL_WORLD_HISTORY]{};
        for (size_t i = 0; i < snapshotCount; i++) {
            times[i] = (float)(netClient.worldHistory.At(i).recvAt - renderAt);
        }
        ImGui::Text("Times:");
        ImGui::PlotHistogram("times", times, (int)snapshotCount, 0, 0, -2.0f, 2.0f, ImVec2(300.0f, 50.0f));
    }
    ImGui::End();
}

void UI::HUD(const Font &font, const Player &player, const DebugStats &debugStats)
{
    assert(spycam);

    const char *text = 0;
    float hudCursorY = 0;

#define PUSH_TEXT(text, color) \
    DrawTextFont(font, text, margin + pad, hudCursorY, 0, 0, font.baseSize, color); \
    hudCursorY += font.baseSize + pad;

    int linesOfText = 8;
    if (debugStats.tick) {
        linesOfText += 10;
    }
    if (debugStats.rtt) {
        linesOfText += 5;
    }
    const float margin = 6.0f;   // left/top margin
    const float pad = 4.0f;      // left/top pad
    const float hudWidth = 240.0f;
    const float hudHeight = linesOfText * (font.baseSize + pad) + pad;

    hudCursorY += margin;

    const Color darkerGray{ 40, 40, 40, 255 };
    UNUSED(darkerGray);
    DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, DARKBLUE);
    DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

    hudCursorY += pad;

    text = SafeTextFormat("%2i fps (%.02f ms)", GetFPS(), GetFrameTime() * 1000.0f);
    PUSH_TEXT(text, WHITE);
    text = SafeTextFormat("Coins: %d", player.inventory.slots[(int)PlayerInventorySlot::Coin_Copper].count);
    PUSH_TEXT(text, YELLOW);
    text = SafeTextFormat("Coins collected   %u", player.stats.coinsCollected);
    PUSH_TEXT(text, LIGHTGRAY);
    text = SafeTextFormat("Damage dealt      %.2f", player.stats.damageDealt);
    PUSH_TEXT(text, LIGHTGRAY);
    text = SafeTextFormat("Kilometers walked %.2f", player.stats.kmWalked);
    PUSH_TEXT(text, LIGHTGRAY);
    text = SafeTextFormat("Slimes slain      %u", player.stats.slimesSlain);
    PUSH_TEXT(text, LIGHTGRAY);
    text = SafeTextFormat("Times fist swung  %u", player.stats.timesFistSwung);
    PUSH_TEXT(text, LIGHTGRAY);
    text = SafeTextFormat("Times sword swung %u", player.stats.timesSwordSwung);
    PUSH_TEXT(text, LIGHTGRAY);

    if (debugStats.tick) {
        text = SafeTextFormat("Tick          %u", debugStats.tick);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("framAccum     %.03f", debugStats.tickAccum);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("frameDt       %.03f", debugStats.frameDt);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Camera speed  %.03f", debugStats.cameraSpeed);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Zoom          %.03f", spycam->GetZoom());
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Zoom inverse  %.03f", spycam->GetInvZoom());
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Zoom mip      %d", spycam->GetZoomMipLevel());
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Tiles visible %zu", debugStats.tilesDrawn);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Particle FX   %zu", debugStats.effectsActive);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Particles     %zu", debugStats.particlesActive);
        PUSH_TEXT(text, GRAY);
    }

    static uint64_t bytesSentStart = 0;
    static uint64_t bytesRecvStart = 0;
    static double bandwidthTimerStartedAt = 0;
    static float kbpsOut = 0.0f;
    static float kbpsIn = 0.0f;

    if (debugStats.rtt) {
        if (!bandwidthTimerStartedAt) {
            bandwidthTimerStartedAt = GetTime();
        }
        if (GetTime() - bandwidthTimerStartedAt >= 1.0) {
            kbpsOut = (float)(debugStats.bytes_sent - bytesSentStart) / 1024.0f;
            kbpsIn = (float)(debugStats.bytes_recv - bytesRecvStart) / 1024.0f;
            bandwidthTimerStartedAt = GetTime();
            bytesSentStart = debugStats.bytes_sent;
            bytesRecvStart = debugStats.bytes_recv;
        }
        text = SafeTextFormat("Ping          %u", debugStats.rtt);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Packets sent  %llu", debugStats.packets_sent);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Packets lost  %u", debugStats.packets_lost);
        PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Bytes sent    %llu", debugStats.bytes_sent);
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Bytes recv    %llu", debugStats.bytes_recv);
        //PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Up         %8.03f kbps", kbpsOut);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Down       %8.03f kbps", kbpsIn);
        PUSH_TEXT(text, GRAY);
    } else {
        bytesSentStart = 0;
        bytesRecvStart = 0;
        bandwidthTimerStartedAt = 0;
        kbpsOut = 0.0f;
        kbpsIn = 0.0f;
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
            text = SafeTextFormatIP(kv.second.address);
            PUSH_TEXT(text, WHITE);
        }
    }
#endif
#undef PUSH_TEXT
}

void UI::QuickHUD(const Font &font, const Player &player, const Tilemap &tilemap)
{
    const char *text = 0;

    int linesOfText = 1;
    const float margin = 6.0f;   // left/top margin
    const float pad = 4.0f;      // left/top pad
    const float hudWidth = 240.0f;
    const float hudHeight = linesOfText * (font.baseSize + pad) + pad;

    float hudCursorY = margin;

    hudCursorY += pad;

    const Spritesheet &gildedSpritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Coin_Gilded);
    const SpriteDef *gildedSpriteDef = gildedSpritesheet.FindSprite("coin");
    Rectangle frameRect{};
    frameRect.x      = (float)gildedSpriteDef->spritesheet->frames[3].x;
    frameRect.y      = (float)gildedSpriteDef->spritesheet->frames[3].y;
    frameRect.width  = (float)gildedSpriteDef->spritesheet->frames[3].width;
    frameRect.height = (float)gildedSpriteDef->spritesheet->frames[3].height;
    DrawTextureRec(gildedSpriteDef->spritesheet->texture, frameRect, { margin + pad, hudCursorY }, WHITE);

    text = SafeTextFormat("%d", player.inventory.slots[(int)PlayerInventorySlot::Coin_Copper].count);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;

    const Vector2 playerBC = player.body.GroundPosition();
    const int16_t playerChunkX = tilemap.CalcChunk(playerBC.x);
    const int16_t playerChunkY = tilemap.CalcChunk(playerBC.y);
    const int16_t playerTileX = tilemap.CalcChunkTile(playerBC.x);
    const int16_t playerTileY = tilemap.CalcChunkTile(playerBC.y);

    text = SafeTextFormat("world: %0.2f %0.2f", playerBC.x, playerBC.y);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;

    text = SafeTextFormat("chunk: %7d %7d", playerChunkX, playerChunkY);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;

    text = SafeTextFormat(" tile: %7d %7d", playerTileX, playerTileY);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;
}

void UI::Chat(const Font &font, int fontSize, World &world, NetClient &netClient, bool processKeyboard, bool &chatActive, bool &escape)
{
    const float margin = 6.0f;   // left/bottom margin
    const float pad = 4.0f;      // left/bottom pad
    const float inputBoxHeight = fontSize + pad * 2.0f;
    const float chatWidth = 800.0f;
    const float chatBottom = screenSize.y - margin - inputBoxHeight;

    // Render chat history
    world.chatHistory.Render(font, fontSize, world, margin, chatBottom, chatWidth, chatActive);

    // Render chat input box
    static GuiTextBoxAdvancedState chatInputState;
    static int chatInputTextLen = 0;
    static char chatInputText[CHATMSG_LENGTH_MAX]{};

    // HACK: Check for this *after* rendering chat to avoid pressing "t" causing it to show up in the chat box
    bool addCommandSlash = false;
    if (!chatActive && processKeyboard && (IsKeyPressed(KEY_T) || IsKeyPressed(KEY_SLASH))) {
        if (IsKeyPressed(KEY_SLASH)) {
            addCommandSlash = true;
        }
        processKeyboard = false;
        chatActive = true;
        GuiSetActiveTextbox(&chatInputState);
    }

    if (chatActive) {
        Rectangle chatInputRect = { margin, screenSize.y - margin - inputBoxHeight, chatWidth, inputBoxHeight };
        if (GuiTextBoxAdvanced(&chatInputState, chatInputRect, chatInputText, &chatInputTextLen, CHATMSG_LENGTH_MAX, !processKeyboard)) {
            if (chatInputTextLen) {
                ErrorType sendResult = netClient.SendChatMessage(chatInputText, chatInputTextLen);
                if (sendResult == ErrorType::NotConnected) {
                    //world.chatHistory.PushSam(CSTR("You're not connected to a server. Nobody is listening. :("));
                    world.chatHistory.PushPlayer(world.playerId, chatInputText, chatInputTextLen);
                }
                chatActive = false;
                memset(chatInputText, 0, sizeof(chatInputText));
                chatInputTextLen = 0;
            }
        }

        if (processKeyboard && escape) {
            chatActive = false;
            memset(chatInputText, 0, sizeof(chatInputText));
            chatInputTextLen = 0;
            escape = false;
        }
    }

    if (addCommandSlash) {
        GuiTextBoxAdvancedPushCodepoint(&chatInputState, chatInputText, &chatInputTextLen, CHATMSG_LENGTH_MAX, '/');
        //rstb_String str = { 0 };
        //str.buffer = chatInputText;
        //str.used = chatInputTextLen;
        //str.capacity = CHATMSG_LENGTH_MAX;
        //stb_textedit_key(&str, (STB_TexteditState *)chatInputState.state, KEY_SLASH);
        //chatInputTextLen = str.used;
    }
}

void UI::TileHoverTip(const Font &font, const Tilemap &map)
{
    const Tile *mouseTile = map.TileAtWorld(mouseWorld.x, mouseWorld.y);
    if (!mouseTile) {
        return;
    }

    const int mouseTileX = (int)mouseWorld.x / TILE_W;
    const int mouseTileY = (int)mouseWorld.y / TILE_W;

    const float tooltipOffsetX = 10.0f;
    const float tooltipOffsetY = 10.0f;
    const float tooltipPad = 4.0f;

    float tooltipX = mouseScreen.x + tooltipOffsetX;
    float tooltipY = mouseScreen.y + tooltipOffsetY;
    const float tooltipW = 220.0f + tooltipPad * 2.0f;
    const float tooltipH = 40.0f + tooltipPad * 2.0f;

    if (tooltipX + tooltipW > screenSize.x) tooltipX = screenSize.x - tooltipW;
    if (tooltipY + tooltipH > screenSize.y) tooltipY = screenSize.y - tooltipH;

    Rectangle tooltipRect{ tooltipX, tooltipY, tooltipW, tooltipH };
    DrawRectangleRec(tooltipRect, Fade(DARKGRAY, 0.8f));
    DrawRectangleLinesEx(tooltipRect, 1, Fade(BLACK, 0.8f));

    int lineOffset = 0;
    DrawTextFont(font, SafeTextFormat("tilePos : %d, %d", mouseTileX, mouseTileY),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, 0, 0, font.baseSize, WHITE);
    lineOffset += font.baseSize;
    DrawTextFont(font, SafeTextFormat("tileSize: %zu, %zu", TILE_W, TILE_W),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, 0, 0, font.baseSize, WHITE);
    lineOffset += font.baseSize;
    DrawTextFont(font, SafeTextFormat("tileType: %d", mouseTile->type),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, 0, 0, font.baseSize, WHITE);
    lineOffset += font.baseSize;
}

int UI::Menu(const Font &font, const char **items, size_t itemCount)
{
    // TODO: Better animations/audio
    // Animations: https://www.youtube.com/watch?v=WjyP7cvZ1Ns
    // Audio: https://www.youtube.com/watch?v=5ewvDbt5U3c

    int itemPressed = -1;

    Vector2 menuPos{};
    Vector2 menuSize{};
    Vector2 menuPad{ 60.0f, 40.0f };
    const float menuSpacingY = 20.0f;
    const float hitboxPadY = 4.0f;

    struct MenuItem {
        const char *text;
        Vector2 size;
        Vector2 offset;  // offset itemX from center line and itemY from top
    } menuItems[UI_MENU_ITEMS_MAX]{};
    assert(itemCount < ARRAY_SIZE(menuItems));

    Vector2 cursor{};
    cursor.y = menuPad.y;
    menuSize.y += menuPad.y;
    for (int i = 0; i < itemCount; i++) {
        menuItems[i].text = items[i];
        menuItems[i].size = MeasureTextEx(font, menuItems[i].text, (float)font.baseSize, font.baseSize/10.0f);
        menuItems[i].offset.x = -menuItems[i].size.x / 2.0f;
        menuItems[i].offset.y = cursor.y;
        cursor.y += menuItems[i].size.y + menuSpacingY;
        menuSize.x = MAX(menuSize.x, menuItems[i].size.x + menuPad.x * 2);
    }
    cursor.y -= menuSpacingY;
    cursor.y += menuPad.y;
    menuSize.y = cursor.y;

    menuPos.x = (screenSize.x - menuSize.x) / 2.0f;
    menuPos.y = (screenSize.y - menuSize.y) / 2.0f;

    Rectangle menuRect{ menuPos.x, menuPos.y, menuSize.x, menuSize.y };
    DrawRectangleRec(menuRect, { 130, 170, 240, 255 }); // Fade(BLACK, 0.7f));
    DrawRectangleLinesEx(menuRect, 2.0f, BLACK);
    const float menuCenterX = menuPos.x + menuSize.x / 2.0f;
    for (int i = 0; i < itemCount; i++) {
        Vector2 itemPos{};
        //itemPos.x = menuCenterX + menuItems[i].offset.x;
        itemPos.x = menuPos.x + menuPad.x;
        itemPos.y = menuPos.y + menuItems[i].offset.y;
        Rectangle hitbox{ menuPos.x + menuPad.x, itemPos.y - hitboxPadY, menuSize.x - menuPad.x * 2, menuItems[i].size.y + hitboxPadY * 2 };
        bool hovered = PointInRect(mouseScreen, hitbox);
        bool pressed = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        if (pressed) {
            itemPressed = i;
        }
        int size = font.baseSize;
        float offsetX = 3.0f;
        float offsetY = 3.0f;
        const float ox = 3.0f;
        const float oy = 5.0f;
        offsetX += CLAMP((mouseScreen.x - (hitbox.x + hitbox.width / 2.0f)) * ox / hitbox.width, -ox, ox);
        offsetY += CLAMP((mouseScreen.y - (hitbox.y + hitbox.height / 2.0f)) * oy / hitbox.height, -oy, oy);

        if (pressed || hovered) {
            itemPos.x += 6.0f;
            offsetX = 3.0f;
            offsetY = 3.0f;
        }
        //DrawRectangleRec(hitbox, Fade(RED, 0.3f + 0.3f * i));
        //DrawRectangleRec({ itemPos.x, itemPos.y, menuItems[i].size.x, menuItems[i].size.y }, Fade(GREEN, 0.3f + 0.3f * i));
        DrawTextFont(font, menuItems[i].text, itemPos.x, itemPos.y, offsetX, offsetY, size, hovered ? pressed ? RED : YELLOW : WHITE);
    }

    return itemPressed;
}

void UI::DearMenu(ImFont *bigFont, bool &escape, bool connectedToServer)
{
    const char *menuId = "EscMenu";
    if (!ImGui::IsPopupOpen(menuId) && escape) {
        ImGui::OpenPopup(menuId);
        escape = false;
    }

    ImVec2 menuCenter {
        screenSize.x / 2.0f,
        screenSize.y / 2.0f
    };
    ImGui::SetNextWindowPos(menuCenter, 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400));

    static enum class Menu {
        Main,
        Audio
    } currentMenu = Menu::Main;

    bool menuOpen = ImGui::BeginPopupModal(menuId, 0,
        //ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    if (menuOpen) {
        switch (currentMenu) {
            case Menu::Main: {
                if (menuOpen && escape) {
                    ImGui::CloseCurrentPopup();
                    escape = false;
                    break;
                }

                ImGui::PushFont(bigFont);
                CenteredText("Main Menu");
                ImGui::MenuItem("Resume");
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::MenuItem("Audio");
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    currentMenu = Menu::Audio;
                }
                if (connectedToServer) {
                    ImGui::MenuItem("Log Off");
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        disconnectRequested = true;
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    ImGui::MenuItem("Quit");
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        quitRequested = true;
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::PopFont();
                break;
            } case Menu::Audio: {
                if ((menuOpen && escape) || ImGui::Button("< Back")) {
                    currentMenu = Menu::Main;
                    escape = false;
                    break;
                }

                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 100);
                static bool audioAdvanced = false;
                if (ImGui::Button(audioAdvanced ? "Show Less" : "Show More", ImVec2(100, 0))) {
                    audioAdvanced = !audioAdvanced;
                }
                ImGui::SameLine();
                ImGui::SetCursorPosY(5);  // idk how to calculate this properly
                ImGui::PushFont(bigFont);
                CenteredText("Audio");
                ImGui::PopFont();

                CenteredSliderFloatLeft("##Master", &Catalog::g_mixer.masterVolume, 0.0f, 1.0f);
                CenteredSliderFloatLeft("##Music ", &Catalog::g_mixer.musicVolume, 0.0f, 1.0f);
                CenteredSliderFloatLeft("##Sfx   ", &Catalog::g_mixer.sfxVolume, 0.0f, 1.0f);

                if (audioAdvanced) {
                    if (ImGui::TreeNode("Tracks")) {
                        for (size_t i = 1; i < (size_t)Catalog::TrackID::Count; i++) {
                            if (ImGui::TreeNode(Catalog::TrackIDString((Catalog::TrackID)i))) {
                                SliderFloatLeft("##vLimit  ", &Catalog::g_tracks.mixer.volumeLimit[i], 0.0f, 1.0f);
                                SliderFloatLeft("##vVolume ", &Catalog::g_tracks.mixer.volume[i], 0.0f, 1.0f);
                                SliderFloatLeft("##vSpeed  ", &Catalog::g_tracks.mixer.volumeSpeed[i], 0.0f, 2.0f);
                                SliderFloatLeft("##vTarget ", &Catalog::g_tracks.mixer.volumeTarget[i], 0.0f, 1.0f);
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Sound FX")) {
                        for (size_t i = 1; i < (size_t)Catalog::SoundID::Count; i++) {
                            if (ImGui::TreeNode(Catalog::SoundIDString((Catalog::SoundID)i))) {
                                SliderFloatLeft("##vLimit  ", &Catalog::g_sounds.mixer.volumeLimit[i], 0.0f, 1.0f);
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                break;
            }
        }
        ImGui::EndPopup();
    }
}

void UI::Inventory(const Texture &invItems, Player& player, NetClient &netClient, bool &escape, bool &inventoryActive)
{
    if (inventoryActive && escape) {
        inventoryActive = false;
        escape = false;
    }

    const ImVec2 inventorySize{ 540.0, 360.0f };
    const float pad = 80.0f;
    const float left = screenSize.x - pad - inventorySize.x;
    const float top = pad;
    ImGui::SetNextWindowPos(ImVec2(left, top), ImGuiCond_Once);
    ImGui::SetNextWindowSize(inventorySize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

    auto bgWindow = BLACK;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(bgWindow.r, bgWindow.g, bgWindow.b, 0.7f * 255.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);

    ImGui::Begin("##inventory", 0,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    static bool ignoreEmpty = false;
    ImGui::Text("Your stuffs:");
    ImGui::SameLine();
    if (ImGui::Button("Sort")) player.inventory.Sort(ignoreEmpty);
    ImGui::SameLine();
    if (ImGui::Button("Sort & Combine")) player.inventory.SortAndCombine(ignoreEmpty);
#if _DEBUG
    auto dbgColor = PINK;
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(dbgColor.r, dbgColor.g, dbgColor.b, 0.7f * 255.0f));
    ImGui::SameLine();
    if (ImGui::Button("Combine")) player.inventory.Combine(ignoreEmpty);
    ImGui::SameLine();
    if (ImGui::Button("Randomize")) player.inventory.Randomize();
    ImGui::SameLine();
    ImGui::Checkbox("Ignore empty", &ignoreEmpty);
    ImGui::PopStyleColor();
#endif

    auto bgColor = DARKBLUE;
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(bgColor.r, bgColor.g, bgColor.b, 0.7f * 255.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 255, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 255, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));

    dlb_rand32_t invRand{};
    dlb_rand32_seed_r(&invRand, 2, 2);

    ItemStack &cursorStack = player.inventory.CursorStack();

    int id = 0;
    for (int row = 0; row < PLAYER_INV_ROWS; row++) {
        ImGui::PushID(row);
        for (int col = 0; col < PLAYER_INV_COLS; col++) {
            ImGui::PushID(col);

            int slot = row * PLAYER_INV_COLS + col;
            ItemStack &invStack = player.inventory.GetInvStack(slot);

            if (invStack.count) {
                Vector2 uv0{};
                Vector2 uv1{};
                player.inventory.TexRect(invItems, invStack.id, uv0, uv1);
                ImGui::ImageButton((ImTextureID)(size_t)invItems.id, ImVec2(ITEM_W, ITEM_H),
                    ImVec2{ uv0.x, uv0.y }, ImVec2{ uv1.x, uv1.y });

                const ImVec2 topLeft = ImGui::GetItemRectMin();
                ImDrawList *drawList = ImGui::GetWindowDrawList();

                char countBuf[16]{};
                snprintf(countBuf, sizeof(countBuf), "%d", invStack.count);
                //snprintf(countBuf, sizeof(countBuf), "%d", newInvStack.count);
                drawList->AddText(
                    { topLeft.x + 4, topLeft.y + 2 },
                    IM_COL32_WHITE,
                    countBuf
                );
            } else {
                ImGui::Button("##inv_slot", ImVec2(48, 48));

                // TODO: Add inv placeholder icon (was grayed out sword in items.png)
                //ImGui::ImageButton((ImTextureID)(size_t)invItems.id, size, uv0, uv1, -1,
                //    ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 0.5f));
            }

            // Terraria:
            //   on mouse down
            //   always bottom right of cursor
            //   left mouse down = take/swap/combine/drop stack
            //   right mouse down = if stack is same (or cursor empty), take 1 item immediately, then repeat with acceleration
            //   sin wave scale bobbing
            //   can use held item as if equipped
            // D2R:
            //   Always centers item on cursor
            //   empty cursor:
            //     left mouse down = take/swap/drop stack (or combine for rare cases like keys)
            // Minecraft:
            //   empty cursor:
            //     left mouse down = pick up stack
            //     right mouse down = pick up bigger half of stack [(int)ceilf(count / 2.0f)]
            //   filled cursor:
            //     left mouse down + drag split stack evenly
            //     right mouse down + drag 1 item piles
            //     mouse up = keep remainder on cursor
            //   Always centers item on cursor
            //   squish (less width, more height) animation on pick-up
            if (ImGui::IsItemHovered()) {
                const int scrollY = (int)GetMouseWheelMove();
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    const bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                    netClient.SendSlotClick(slot, doubleClick);
                } else if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    // if cursor empty, pick up half (or 1 like terraria? hmm..)
                    // else, pick up 1 more (repeat w/ acceleration)
                } else if (scrollY) {
                    // TODO(dlb): This will break if the window has any scrolling controls
                    netClient.SendSlotScroll(slot, scrollY);
                } else if (IsKeyPressed(KEY_Q)) {
                    uint32_t dropCount = 1;
                    if (IsKeyDown(KEY_LEFT_CONTROL)) {
                        dropCount = invStack.count;
                    }
                    netClient.SendSlotDrop(slot, dropCount);
                }
#if _DEBUG
                else if (IsKeyPressed(KEY_KP_ADD)) {
                    invStack.id = (Catalog::ItemID)(((int)invStack.id + 1) % (int)Catalog::ItemID::Count);
                } else if (IsKeyPressed(KEY_KP_SUBTRACT)) {
                    invStack.id = (Catalog::ItemID)(((int)invStack.id - 1 + (int)Catalog::ItemID::Count) % (int)Catalog::ItemID::Count);
                }
#endif

                if (invStack.count && !cursorStack.count) {
                    const char *invName = invStack.Name();
                    ImGui::SetTooltip("%u %s", invStack.count, invName);
                }
            }

            if (col < PLAYER_INV_COLS - 1) {
                ImGui::SameLine();
            }
            ImGui::PopID();
        }
        ImGui::PopID();
    }
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(6);

    if (cursorStack.count) {
#if CURSOR_ITEM_HIDES_POINTER
        HideCursor();
#endif

        ImDrawList *drawList = ImGui::GetForegroundDrawList();
        drawList->PushClipRect({0, 0}, {screenSize.x, screenSize.y});

        Vector2 uv0{};
        Vector2 uv1{};
        player.inventory.TexRect(invItems, cursorStack.id, uv0, uv1);

        Vector2 cursorOffset{};
#if CURSOR_ITEM_RELATIVE_TERRARIA
        cursorOffset.x = 8;
        cursorOffset.y = 8;
#else
        cursorOffset.x = -(ITEM_W / 2);
        cursorOffset.y = -(ITEM_H / 2);
#endif

#if 1
        const Rectangle dstRect{
           mouseScreen.x + cursorOffset.x,
           mouseScreen.y + cursorOffset.y,
           32.0f,
           32.0f
        };
#else
        // NOTE: This is because there's a fair time gap between EndDrawing() (which calls glfwPollInput
        // internally) and this code running (e.g. networking happens) during which the mouse can still be
        // moving. Perhaps it makes sense to always just re-query mouse position rather than using the
        // Raylib CORE.input cache for any UI stuff?
        GLFWwindow *glfwWindow = glfwGetCurrentContext();
        double freshX, freshY;
        glfwGetCursorPos(glfwWindow, &freshX, &freshY);
        const Rectangle dstRect{
           (float)freshX + cursorOffset.x,
           (float)freshY + cursorOffset.y,
           32.0f,
           32.0f
        };
#endif

#if 1
        if (uv0.x >= 0.0f && uv0.x < 1.0f &&
            uv0.y >= 0.0f && uv0.y < 1.0f &&
            uv1.x > 0.0f && uv1.x <= 1.0f &&
            uv1.y > 0.0f && uv1.y <= 1.0f)
        {
            drawList->AddImage(
                (ImTextureID)(size_t)invItems.id,
                ImVec2(dstRect.x, dstRect.y),
                ImVec2(dstRect.x + dstRect.width, dstRect.y + dstRect.height),
                ImVec2{ uv0.x, uv0.y }, ImVec2{ uv1.x, uv1.y }
            );
        } else {
            drawList->AddRectFilled(
                ImVec2(dstRect.x - (ITEM_W / 2), dstRect.y - (ITEM_H / 2)),
                ImVec2(dstRect.x + (ITEM_W / 2), dstRect.y + (ITEM_H / 2)),
                IM_COL32(PINK.r, PINK.g, PINK.b, PINK.a)
            );
        }
#else
        drawList->AddImage(
            (ImTextureID)(size_t)invItems.id,
            ImVec2(dstRect.x, dstRect.y),
            ImVec2(dstRect.x + dstRect.width, dstRect.y + dstRect.height),
            ImVec2{ uv0.x, uv0.y }, ImVec2{ uv1.x, uv1.y }
        );
#endif

        char countBuf[16]{};
        snprintf(countBuf, sizeof(countBuf), "%d", cursorStack.count);
        drawList->AddText({ dstRect.x - 4, dstRect.y - 6 }, IM_COL32_WHITE, countBuf);

        drawList->PopClipRect();
    } else {
#if CURSOR_ITEM_HIDES_POINTER
        ShowCursor();
#endif
    }

    ImGui::End();
}