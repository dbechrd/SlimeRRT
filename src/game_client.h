#pragma once
#include "args.h"
#include "error.h"
#include "net_client.h"
#include "spycam.h"

struct GameServer;
struct ImFont;
struct World;

struct GameClient {
    Args       *args        {};
    GameServer *localServer {};
    NetClient   netClient   {};

    GameClient(Args *args) : args(args) {}
    ErrorType Run(void);

private:
    static const char *LOG_SRC;

    // Flags
    bool inventoryActive = false;

    // Textures
    Texture checkboardTexture {};

    // Shaders
    Shader pixelFixer {};
    int    pixelFixerScreenSizeUniformLoc {};

    // Other important stuff
    Vector2 screenSize {};

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