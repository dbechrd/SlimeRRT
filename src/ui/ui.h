#pragma once

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
        uint32_t ping;
        float cameraSpeed;
        size_t tilesDrawn;
        size_t effectsActive;
        size_t particlesActive;
    };

    static void Begin(
        Font    *font,
        Vector2 mouseScreen,
        Vector2 mouseWorld,
        Vector2 screenRect,
        Spycam  *spycam
    ) {
        UI::font        = font;
        UI::mouseScreen = mouseScreen;
        UI::mouseWorld  = mouseWorld;
        UI::screenRect  = screenRect;
        UI::spycam      = spycam;
    };

    static void LoginForm(NetClient &netClient, ImGuiIO& io, bool &escape);
    static void Mixer(void);
    static void WorldGrid(const Tilemap &map);
    static void TileHoverOutline(const Tilemap &map);
    static void TileHoverTip(const Tilemap &map);
    static void Minimap(const World &world);
    static void HUD(const Player &player, const DebugStats &debugStats);

private:
    static Font    *font;
    static Vector2 mouseScreen;
    static Vector2 mouseWorld;
    static Vector2 screenRect;
    static Spycam  *spycam;

    static void CenteredText(const char *text, const char *textToMeasure);
};
