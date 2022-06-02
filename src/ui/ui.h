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
        float cameraSpeed;
        size_t tilesDrawn;
        size_t effectsActive;
        size_t particlesActive;

        uint32_t rtt;
        uint64_t packets_sent;
        uint32_t packets_lost;
        uint64_t bytes_sent;
        uint64_t bytes_recv;
    };

    static void Begin(Vector2 screenSize, Spycam *spycam);
    static void HandleInput(const PlayerControllerState &input);
    static bool DisconnectRequested(bool connectedToServer);
    static bool QuitRequested();

    // World UI
    static void TileHoverOutline(const Tilemap &map);
    static void WorldGrid();

    // Screen UI
    static void Minimap(const Font &font, World &world);
    static void Menubar(const NetClient &netClient);
    static void ShowDemoWindow();
    static void LoginForm(NetClient &netClient, bool &escape);
    static void Mixer(void);
    static void ParticleConfig(World &world);
    static void Netstat(NetClient &netClient, double renderAt);
    static void HUD(const Font &font, World &world, const DebugStats &debugStats);
    static void QuickHUD(const Font &font, const Player &player, const Tilemap &tilemap);
    static void Chat(const Font &font, int fontSize, World &world, NetClient &netClient, bool processKeyboard, bool &chatActive, bool &escape);
    static void TileHoverTip(const Font &font, const Tilemap &map);
    static int  OldRaylibMenu(const Font &font, const char **items, size_t itemCount);
    static void MainMenu(bool &escape, const Args &args, NetClient &netClient);
    static void InGameMenu(bool &escape, bool connectedToServer);
    static void Inventory(const Texture &invItems, Player &player, NetClient &netClient, bool &escape, bool &inventoryActive);

private:
    static Vector2 mouseScreen;
    static Vector2 mouseWorld;
    static Vector2 screenSize;
    static Spycam  *spycam;

    static bool showMenubar;
    static bool showDemoWindow;
    static bool showLoginWindow;
    static bool showParticleConfig;

    static bool disconnectRequested;
    static bool quitRequested;


    enum Menu : int {
        Main,
        Singleplayer,
        Multiplayer,
        Audio
    };
    static Menu mainMenu;

    static void CenteredText(const char *text);
    static void SliderFloatLeft(const char *label, float *v, float min, float max);
    static void SliderIntLeft(const char *label, int *v, int min, int max);
    static bool BreadcrumbButton(const char *label, Menu menu, bool &escape);
    static void BreadcrumbText(const char *text);
    static bool MenuBackButton(Menu menu, bool &escape);
    static bool MenuItemClick(const char *label);
};
