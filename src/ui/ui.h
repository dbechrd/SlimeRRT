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
        uint32_t cl_input_seq;
        uint32_t sv_input_ack;
        uint32_t input_buf_size;
    };

    static void Update(const PlayerControllerState &input, Vector2 screenSize, Spycam *spycam);
    static bool DisconnectRequested(bool disconnected);
    static bool QuitRequested();

    // World UI
    static void TileHoverOutline(const Tilemap &map);
    static void WorldGrid();

    // Screen UI
    static void Minimap(const Font &font, World &world);
    static void Menubar(const NetClient &netClient);
    static void ShowDemoWindow(void);
    static void ParticleConfig(World &world);
    static void Netstat(NetClient &netClient, double renderAt);
    static void ItemProtoEditor(World &world);
    static void HUD(const Font &font, World &world, const DebugStats &debugStats);
    static void QuickHUD(const Font &font, const Player &player, const Tilemap &tilemap);
    static void Chat(const Font &font, int fontSize, World &world, NetClient &netClient, bool &escape);
    static void TileHoverTip(const Font &font, const Tilemap &map);
    static int  OldRaylibMenu(const Font &font, const char **items, size_t itemCount);
    static void MainMenu(bool &escape, GameClient &game);
    static void InGameMenu(bool &escape, bool connectedToServer);
    static void InventorySlot(bool inventoryActive, int slot, const Texture &invItems, Player &player, NetClient &netClient);
    static void Inventory(const Texture &invItems, Player &player, NetClient &netClient, bool &escape, bool &inventoryActive);
    static void Dialog(World &world);
    static void ParticleText(Vector2 pos, const char *text);

private:
    static const PlayerControllerState *input;
    static Vector2 mouseScreen;
    static Vector2 mouseWorld;
    static Vector2 screenSize;
    static Spycam  *spycam;

    static bool showMenubar;
    static bool showDemoWindow;
    static bool showParticleConfig;
    static bool showNetstatWindow;
    static bool showItemProtoEditor;

    static bool disconnectRequested;
    static bool quitRequested;

    enum Menu : int {
        Main,
        Singleplayer,
        Multiplayer,
        Multiplayer_New,
        Audio
    };
    static Menu mainMenu;
    static const char *hoverLabel;
    static const char *prevHoverLabel;

    static void MenuTitle(const char *text);
    static void MenuTitle(const ImVec4 &color, const char *text);
    static void SliderFloatLeft(const char *label, float *v, float min, float max);
    static void SliderIntLeft(const char *label, int *v, int min, int max);

    enum MyItemColumnID {
        ItemColumn_Unknown,
        ItemColumn_ID,
        ItemColumn_Class,
        ItemColumn_StackLimit,
        ItemColumn_Value,
        ItemColumn_DamageFlatMin,
        ItemColumn_DamageFlatMax,
        ItemColumn_Name,
        ItemColumn_NamePlural,
    };

    static const ImGuiTableSortSpecs *itemSortSpecs;
    static int  ItemCompareWithSortSpecs(const void *lhs, const void *rhs);

    static bool BreadcrumbButton(const char *label, Menu menu, bool *escape);
    static void BreadcrumbText(const char *text);
    static bool MenuBackButton(Menu menu, bool *escape);
    static bool MenuItemClick(const char *label);
    static void InventoryItemTooltip(ItemStack &invStack, int slot, Player &player, NetClient &netClient);
};
