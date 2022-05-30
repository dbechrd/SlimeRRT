#pragma once
#include "args.h"
#include "error.h"
#include "net_client.h"
#include "raylib/raygui.h"

struct World;

struct GameClient{
    GameClient(const Args &args) : args(args) {};
    ErrorType Run(void);

private:
    static const char *LOG_SRC;
    const Args &args;
    World      *world    {};
    NetClient  netClient {};

    double frameStart     {};

    const double tickDt = 1.0 / SV_TICK_RATE;
    const double tickDtMax = tickDt * 2.0;
    double       tickAccum {};

    const double sendInputDt = 1.0 / CL_INPUT_SEND_RATE;
    const double sendInputDtMax = sendInputDt * 2.0;
    double       sendInputAccum {};
    InputMode    inputMode = INPUT_MODE_PLAY;

    bool gifRecording = false;
    bool chatVisible = false;
    bool inventoryActive = false;

    Texture checkboardTexture {};
    Spycam spycam {};
    Vector2 screenSize {};
    Shader pixelFixer {};
    int pixelFixerScreenSizeUniformLoc {};

    void                  Init                    (void);
    void                  PlayMode_Audio          (double frameDt);
    PlayerControllerState PlayMode_PollController (void);
    void                  PlayMode_Update         (const PlayerControllerState &input);
    void                  PlayMode_HandleInput    (const PlayerControllerState &input);
    void                  PlayMode_UpdateCamera   (const PlayerControllerState &input, double frameDt);
    void                  PlayMode_DrawWorld      (bool tileHover);
    void                  PlayMode_DrawScreen     (bool tileHover);
};