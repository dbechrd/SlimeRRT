#pragma once

#define UI_MENU_ITEMS_MAX 64

struct ImGuiIO;
struct NetClient;
struct Spycam;
struct Tilemap;
struct World;

struct UI {
    struct DebugStats {
        uint32_t tick;
        double tickAccum;
        double frameDt;
        int    cameraSpeed;
        int    tilesDrawn;
        size_t effectsActive;
        size_t particlesActive;

        uint32_t rtt;
        uint64_t packets_sent;
        uint32_t packets_lost;
        uint64_t bytes_sent;
        uint64_t bytes_recv;
    };

    static void Begin(
        Vector2i mouseScreen,
        Vector2i mouseWorld,
        Vector2i screenSize,
        Spycam  *spycam
    ) {
        UI::mouseScreen = mouseScreen;
        UI::mouseWorld  = mouseWorld;
        UI::screenSize  = screenSize;
        UI::spycam      = spycam;
    };

    // World UI
    static void TileHoverOutline(const Tilemap &map);
    static void WorldGrid(const Tilemap &map);

    // Screen UI
    static void Minimap(const Font &font, const World &world);
    static void LoginForm(NetClient &netClient, ImGuiIO& io, bool &escape);
    static void Mixer(void);
    static void Netstat(NetClient &netClient, double renderAt);
    static void HUD(const Font &font, const Player &player, const DebugStats &debugStats);
    static void Chat(const Font &font, World &world, NetClient &netClient, bool processKeyboard, bool &chatActive, bool &escape);
    static void TileHoverTip(const Font &font, const Tilemap &map);
    static int Menu(const Font &font, bool &escape, bool &exiting, const char **items, size_t itemCount);

private:
    static Vector2i mouseScreen;
    static Vector2i mouseWorld;
    static Vector2i screenSize;
    static Spycam   *spycam;

    static void CenteredText(const char *text);
};
