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
    static void TileHoverOutline(Tilemap &map);
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
    static void TileHoverTip(const Font &font, Tilemap &map);
    static int  OldRaylibMenu(const Font &font, const char **items, size_t itemCount);
    static void MainMenu(bool &escape, GameClient &game);
    static void InGameMenu(bool &escape, bool connectedToServer);
    static void InventorySlot(bool inventoryActive, int slot, const Texture &invItems, Player &player, NetClient &netClient);
    static void Inventory(const Texture &invItems, Player &player, NetClient &netClient, bool &escape, bool &inventoryActive);
    static void Dialog(World &world);
    static void ParticleText(Vector2 pos, const char *text);

private:
    static inline const char *LOG_SRC = "UI";
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

    enum MenuID {
        Menu_Null,
        Menu_Quit,
        Menu_Main,
        Menu_Singleplayer,
        Menu_Multiplayer,
        Menu_Multiplayer_EditServer,
        Menu_Audio
    };
    struct Menu {
        MenuID id;
        const char *name;  // menu button and breadcrumb
        const char *title; // title at top of this menu when active
    };
    class MenuStack {
    public:
        MenuStack(void) { menus[0] = { Menu_Main, "Main Menu", "Slime Game" }; count = 1; }
        bool Push(MenuID id, const char *name, const char *title) {
            if (count < ARRAY_SIZE(menus)) {
                menus[count++] = { id, name, title };
                return true;
            }
            return false;
        }
        void Begin(void) {
            prevBeginID = beginID;
            beginID = menuStack.Last().id;
        }
        bool Changed(void) {
            return Last().id != beginID;
        }
        bool ChangedSinceLastFrame(void) {
            return beginID != prevBeginID;
        }
        const Menu &Get(int index) {
            if (index < 0 || index >= count) {
                DLB_ASSERT(!"Out of bounds");
                index = 0;
            }
            return menus[index];
        }
        const Menu &Last(void) {
            return menus[count - 1];
        }
        bool Pop(void) {
            if (count > 1) {
                menus[count--] = {};
                return true;
            }
            return false;
        }
        void BackTo(MenuID id) {
            while (menuStack.Last().id != id) {
                if (!menuStack.Pop()) break;
            };
        }
        int Count(void) { return count; }
    private:
        Menu   menus       [8]{};   // menu stack data
        int    count       {};      // current size of menu stack
        MenuID beginID     {};      // id of menu active at frame start
        MenuID prevBeginID {};      // id of menu active at previous frame start
    };
    static MenuStack menuStack;
    static const char *hoverLabel;
    static const char *prevHoverLabel;

    static void MenuTitle(const char *text, const ImVec4 &color = { 0, 1, 0, 1 });
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

    static bool BreadcrumbButton(const char *label);
    static void Breadcrumbs(void);
    static bool MenuBackButton(MenuID menu);
    static bool MenuButton(const char *label, const ImVec2 &size = { 600, 0 });
    static void MenuMultiplayer(GameClient &game);
    static void MenuMultiplayerNew(NetClient &netClient);
    static void InventoryItemTooltip(ItemStack &invStack, int slot, Player &player, NetClient &netClient);
};
