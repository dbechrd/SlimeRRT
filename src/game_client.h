#pragma once
#include "args.h"
#include "error.h"
#include "input_mode.h"
#include "net_client.h"
#include "spycam.h"
#include "raylib/raygui.h"

struct GameServer;
struct ImFont;
struct World;

struct GameClient {
    Args       &args;
    GameServer *localServer {};
    NetClient  netClient    {};

    GameClient(Args &args) : args(args) {};
    ErrorType Run(void);

private:
    static const char *LOG_SRC;

    // Time
    double       frameStart {};
    const double tickDt = 1.0 / SV_TICK_RATE;
    const double tickDtMax = tickDt * 2.0;
    const double sendInputDt = 1.0 / CL_INPUT_SEND_RATE;
    const double sendInputDtMax = sendInputDt * 2.0;

    // Flags
    InputMode inputMode = INPUT_MODE_PLAY;
    bool chatVisible = false;
    bool inventoryActive = false;

    // Textures
    Texture checkboardTexture {};

    // Shaders
    Shader pixelFixer {};
    int    pixelFixerScreenSizeUniformLoc {};

    // Other important stuff
    Vector2 screenSize {};
    Spycam  spycam     {};

    // Other random stuff
    size_t tilesDrawn {};
    double renderAt   {};

    void LoadingScreen           (const char *text);
    void Init                    (void);
    void PlayMode_PollController (PlayerControllerState &input);
    ErrorType PlayMode_Network   (void);
    void PlayMode_Audio          (double frameDt);
    void PlayMode_HandleInput    (PlayerControllerState &input);
    void PlayMode_UpdateCamera   (double frameDt, PlayerControllerState &input);
    void PlayMode_Update         (double frameDt, PlayerControllerState &input);
    void PlayMode_DrawWorld      (PlayerControllerState &input);
    void PlayMode_DrawScreen     (double frameDt, PlayerControllerState &input);
};