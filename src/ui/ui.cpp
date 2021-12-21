#include "error.h"
#include "net_client.h"
#include "../spycam.h"
#include "../tilemap.h"
#include "../world.h"
#include "imgui/imgui.h"
#include "GLFW/glfw3.h"
#include "raylib/raylib.h"

Font    *UI::font;
Vector2 UI::mouseScreen;
Vector2 UI::mouseWorld;
Vector2 UI::screenSize;
Spycam  *UI::spycam;

void UI::TileHoverOutline(const Tilemap &map)
{
    assert(spycam);

    int mouseTileX = 0;
    int mouseTileY = 0;
    Tile *mouseTile = map.TileAtWorldTry(mouseWorld.x, mouseWorld.y, &mouseTileX, &mouseTileY);
    if (!mouseTile) {
        return;
    }

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

void UI::WorldGrid(const Tilemap &map)
{
    for (uint32_t y = 0; y <= map.height; y++) {
        DrawLine(0, y * TILE_H, map.width * TILE_W, y * TILE_H, BLACK);
    }
    for (uint32_t x = 0; x <= map.width; x++) {
        DrawLine(x * TILE_W, 0, x * TILE_W, map.height * TILE_H, BLACK);
    }
    const int center_y = map.height / 2 * TILE_H;
    const int center_x = map.width / 2 * TILE_W;
    DrawLineEx({ 0, (float)center_y }, { (float)map.width * TILE_W, (float)center_y }, 3.0f, RED);
    DrawLineEx({ (float)center_x, 0 }, { (float)center_x, (float)map.height * TILE_H }, 3.0f, GREEN);
}

void UI::Minimap(const World &world)
{
    assert(font);

    // Render minimap
    const int minimapMargin = 6;
    const int minimapBorderWidth = 1;
    const int minimapX = (int)screenSize.x - minimapMargin - world.map->minimap.width - minimapBorderWidth * 2;
    const int minimapY = minimapMargin;
    const int minimapW = world.map->minimap.width + minimapBorderWidth * 2;
    const int minimapH = world.map->minimap.height + minimapBorderWidth * 2;
    const int minimapTexX = minimapX + minimapBorderWidth;
    const int minimapTexY = minimapY + minimapBorderWidth;
    DrawRectangleLines(minimapX, minimapY, minimapW, minimapH, BLACK);
    DrawTexture(world.map->minimap, minimapTexX, minimapTexY, WHITE);

    // Draw slimes on map
    for (size_t i = 0; i < ARRAY_SIZE(world.slimes); i++) {
        const Slime &s = world.slimes[i];
        if (s.id) {
            float x = (s.body.position.x / (world.map->width * TILE_W)) * minimapW + minimapX;
            float y = (s.body.position.y / (world.map->height * TILE_W)) * minimapH + minimapY;
            DrawCircle((int)x, (int)y, 2.0f, Color{ 0, 170, 80, 255 });
        }
    }

    // Draw players on map
    for (size_t i = 0; i < ARRAY_SIZE(world.players); i++) {
        const Player &p = world.players[i];
        if (p.id) {
            float x = (p.body.position.x / (world.map->width * TILE_W)) * minimapW + minimapX;
            float y = (p.body.position.y / (world.map->height * TILE_W)) * minimapH + minimapY;
            const Color playerColor{ 220, 90, 20, 255 };
            DrawCircle((int)x, (int)y, 2.0f, playerColor);
            const char *pName = TextFormat("%.*s", p.nameLength, p.name);
            int nameWidth = MeasureText(pName, font->baseSize);
            DrawTextFont(*font, pName, x - (float)(nameWidth / 2), y - font->baseSize - 4, font->baseSize, YELLOW);
        }
    }
}

void UI::CenteredText(const char *text)
{
    float windowWidth = ImGui::GetWindowSize().x;
    float textWidth = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text(text);
}

void UI::LoginForm(NetClient &netClient, ImGuiIO& io, bool &escape)
{
    if (netClient.IsConnected()) {
        ImGui::CloseCurrentPopup();
    } else {
        ImGui::SetNextWindowSize(ImVec2(240, 0));
        ImGui::SetNextWindowPos(ImVec2(6, 340));
        auto rayDarkBlue = DARKBLUE;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(rayDarkBlue.r, rayDarkBlue.g, rayDarkBlue.b, rayDarkBlue.a));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);
        ImGui::Begin("##mini_menu", 0,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse
        );
        ImGui::PopStyleColor(2);
        ImGui::Text("Play with friends!");
        if (ImGui::Button("Connect to DandyNet", ImVec2(150, 0))) {
            ImGui::OpenPopup("Connect to Server##login_window");
        }

        if (ImGui::BeginPopupModal("Connect to Server##login_window", 0, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char host[SV_HOSTNAME_LENGTH_MAX + 1]{ "slime.theprogrammingjunkie.com" };
            static int  port = SV_DEFAULT_PORT;
            static char username[USERNAME_LENGTH_MAX + 1]{ "guest" };
            static char password[PASSWORD_LENGTH_MAX + 1]{ "pizza" };
            static const char *message = 0;
            bool formValid = true;

            ImGui::Text("    Host:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(232);
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
                    }
                    if (passwordLen < PASSWORD_LENGTH_MIN || passwordLen > PASSWORD_LENGTH_MAX) {
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

            bool closePopup = escape;

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
            if (cancel || IsKeyPressed(io.KeyMap[ImGuiKey_Escape])) {
                closePopup = true;
            }

            ImGui::EndDisabled();

            if (closePopup) {
                ImGui::CloseCurrentPopup();
                memset(username, 0, sizeof(username));
                memset(password, 0, sizeof(password));
                escape = false;
            }

            ImGui::EndPopup();
        }
        ImGui::End();
    }
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

void UI::HUD(const Player &player, const DebugStats &debugStats)
{
    assert(font);
    assert(spycam);

    const char *text = 0;
    float hudCursorY = 0;

#define PUSH_TEXT(text, color) \
    DrawTextFont(*font, text, margin + pad, hudCursorY, font->baseSize, color); \
    hudCursorY += font->baseSize + pad; \

    int linesOfText = 8;
    if (debugStats.tick) {
        linesOfText += 10;
        if (debugStats.ping) {
            linesOfText++;
        }
    }
    const float margin = 6.0f;   // left/top margin
    const float pad = 4.0f;      // left/top pad
    const float hudWidth = 240.0f;
    const float hudHeight = linesOfText * (font->baseSize + pad) + pad;

    hudCursorY += margin;

    const Color darkerGray{ 40, 40, 40, 255 };
    UNUSED(darkerGray);
    DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, DARKBLUE);
    DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

    hudCursorY += pad;

    text = TextFormat("%2i fps (%.02f ms)", GetFPS(), GetFrameTime() * 1000.0f);
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

    if (debugStats.tick) {
        text = TextFormat("Tick          %u", debugStats.tick);
        PUSH_TEXT(text, GRAY);
        text = TextFormat("framAccum     %.03f", debugStats.tickAccum);
        PUSH_TEXT(text, GRAY);
        text = TextFormat("frameDt       %.03f", debugStats.frameDt);
        PUSH_TEXT(text, GRAY);
        if (debugStats.ping) {
            text = TextFormat("lastRTT       %u", debugStats.ping);
            PUSH_TEXT(text, GRAY);
        }
        text = TextFormat("Camera speed  %.03f", debugStats.cameraSpeed);
        PUSH_TEXT(text, GRAY);
        text = TextFormat("Zoom          %.03f", spycam->GetZoom());
        PUSH_TEXT(text, GRAY);
        text = TextFormat("Zoom inverse  %.03f", spycam->GetInvZoom());
        PUSH_TEXT(text, GRAY);
        text = TextFormat("Zoom mip      %d", spycam->GetZoomMipLevel());
        PUSH_TEXT(text, GRAY);
        text = TextFormat("Tiles visible %zu", debugStats.tilesDrawn);
        PUSH_TEXT(text, GRAY);
        text = TextFormat("Particle FX   %zu", debugStats.effectsActive);
        PUSH_TEXT(text, GRAY);
        text = TextFormat("Particles     %zu", debugStats.particlesActive);
        PUSH_TEXT(text, GRAY);
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
}

void UI::Chat(World &world, NetClient &netClient, bool processKeyboard, bool &chatActive, bool &escape)
{
    const float margin = 6.0f;   // left/bottom margin
    const float pad = 4.0f;      // left/bottom pad
    const float inputBoxHeight = font->baseSize + pad * 2.0f;
    const float chatWidth = 800.0f;
    const float chatBottom = screenSize.y - margin - inputBoxHeight;

    // Render chat history
    world.chatHistory.Render(*font, margin, chatBottom, chatWidth, chatActive);

    // Render chat input box
    static GuiTextBoxAdvancedState chatInputState;
    if (chatActive) {
        static int chatInputTextLen = 0;
        static char chatInputText[CHATMSG_LENGTH_MAX]{};

        Rectangle chatInputRect = { margin, screenSize.y - margin - inputBoxHeight, chatWidth, inputBoxHeight };
        if (GuiTextBoxAdvanced(&chatInputState, chatInputRect, chatInputText, &chatInputTextLen, CHATMSG_LENGTH_MAX, !processKeyboard)) {
            if (chatInputTextLen) {
                ErrorType sendResult = netClient.SendChatMessage(chatInputText, chatInputTextLen);
                switch (sendResult) {
                    case ErrorType::NotConnected:
                    {
                        world.chatHistory.PushMessage(CSTR("Sam"), CSTR("You're not connected to a server. Nobody is listening. :("));
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
    if (!chatActive && processKeyboard && IsKeyDown(KEY_T)) {
        chatActive = true;
        GuiSetActiveTextbox(&chatInputState);
    }
}

void UI::TileHoverTip(const Tilemap &map)
{
    assert(font);

    int mouseTileX = 0;
    int mouseTileY = 0;
    Tile *mouseTile = map.TileAtWorldTry(mouseWorld.x, mouseWorld.y, &mouseTileX, &mouseTileY);
    if (!mouseTile) {
        return;
    }

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
    DrawTextFont(*font, TextFormat("tilePos : %d, %d", mouseTileX, mouseTileY),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, font->baseSize, WHITE);
    lineOffset += font->baseSize;
    DrawTextFont(*font, TextFormat("tileSize: %zu, %zu", TILE_W, TILE_W),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, font->baseSize, WHITE);
    lineOffset += font->baseSize;
    DrawTextFont(*font, TextFormat("tileType: %d", mouseTile->tileType),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, font->baseSize, WHITE);
    lineOffset += font->baseSize;
}
