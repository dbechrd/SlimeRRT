#pragma once
#include "args.h"
#include "error.h"
#include "input_mode.h"
#include "net_client.h"
#include "spycam.h"
#include "raylib/raygui.h"

struct ImFont;
struct World;

struct GameClient{
    GameClient(const Args &args) : args(args) {};
    ErrorType Run(void);

private:
    static const char *LOG_SRC;

    // Pirate exclamations
    const Args &args;

    // Time
    double       frameStart {};
    const double tickDt = 1.0 / SV_TICK_RATE;
    const double tickDtMax = tickDt * 2.0;
    double       tickAccum {};
    const double sendInputDt = 1.0 / CL_INPUT_SEND_RATE;
    const double sendInputDtMax = sendInputDt * 2.0;
    double       sendInputAccum {};

    // Flags
    InputMode inputMode = INPUT_MODE_PLAY;
    bool chatVisible = false;
    bool inventoryActive = false;

    // Fonts
    ImFont *imFontHack16 {};
    ImFont *imFontHack32 {};
    ImFont *imFontHack48 {};
    ImFont *imFontHack64 {};
    Font fontSmall {};
    Font fontSdf24 {};
    Font fontSdf72 {};

    // Textures
    Texture checkboardTexture {};

    // Shaders
    Shader pixelFixer {};
    int    pixelFixerScreenSizeUniformLoc {};

    // Other important stuff
    Vector2    screenSize {};
    Spycam     spycam     {};
    World     *world      {};
    NetClient  netClient  {};

    // Other random stuff
    size_t tilesDrawn {};

    void Init                    (void);
    void PlayMode_PollController (PlayerControllerState &input);
    ErrorType PlayMode_Network        (double frameDt, PlayerControllerState &input);
    void PlayMode_Audio          (double frameDt, PlayerControllerState &input);
    void PlayMode_HandleInput    (double frameDt, PlayerControllerState &input);
    void PlayMode_UpdateCamera   (double frameDt, PlayerControllerState &input);
    void PlayMode_Update         (double frameDt, PlayerControllerState &input);
    void PlayMode_DrawWorld      (double frameDt, PlayerControllerState &input);
    void PlayMode_DrawScreen     (double frameDt, PlayerControllerState &input);
};