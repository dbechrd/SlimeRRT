#pragma once
#include "dlb_types.h"

struct PlayerControllerState {
    // Global keys
    bool escape           {};
    bool toggleFullscreen {};
    bool screenshot       {};
    bool dbgToggleVsync   {};

    // Networked input
    bool     walkNorth  {};
    bool     walkEast   {};
    bool     walkSouth  {};
    bool     walkWest   {};
    bool     run        {};
    bool     attack     {};
    uint32_t selectSlot {};  // PlayerInventorySlot
    // Non-networked input
    bool toggleInventory{};

    // Freecam
    bool  cameraNorth      {};
    bool  cameraEast       {};
    bool  cameraSouth      {};
    bool  cameraWest       {};
    bool  cameraRotateCW   {};
    bool  cameraRotateCCW  {};
    bool  cameraZoomOut    {};
    float cameraZoomDelta  {};
    float cameraSpeedDelta {};
    bool  cameraReset      {};

    // Debug
    bool dbgFindMouseTile {};
    bool dbgChatMessage   {};
    bool dbgTeleport      {};
    bool dbgSpawnSam      {};
    bool dbgToggleFreecam {};
    bool dbgNextRtreeRect {};
    bool dbgKillRtreeRect {};
    bool dbgImgui         {};
    bool dbgJump          {};

    void Query(bool processMouse, bool processKeyboard, bool freeCamera);
};

struct InputSample {
    uint32_t seq        {};  // monotonic input sequence number
    uint32_t ownerId    {};  // player who generated this input
    //uint32_t clientTick {};  // client tick when input was detected
    uint8_t  msec       {};  // duration of command (https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization)
    bool     walkNorth  {};
    bool     walkEast   {};
    bool     walkSouth  {};
    bool     walkWest   {};
    bool     run        {};
    bool     attack     {};
    uint32_t selectSlot {};  // PlayerInventorySlot
    bool     skipFx     {};  // once the input has been processed once, don't trigger FX (particles, sounds, etc.)

    void FromController(uint32_t playerId, uint32_t inputSeq, double frameDt, const PlayerControllerState &controllerState)
    {
        double frameDtMs = frameDt * 1000.0;
        if (frameDtMs > UINT8_MAX) {
            TraceLog(LOG_WARNING, "InputSample msec too large, will be truncated to 256 ms");
        }
        seq        = inputSeq;
        ownerId    = playerId;
        //clientTick = tick;
        msec       = (uint8_t)MIN(frameDt * 1000.0, UINT8_MAX);
#if CL_DEBUG_SPEEDHAX
        if (g_inputMsecHax) {
            msec = MIN(g_inputMsecHax, UINT8_MAX);
        }
#endif
        walkNorth  = controllerState.walkNorth;
        walkEast   = controllerState.walkEast;
        walkSouth  = controllerState.walkSouth;
        walkWest   = controllerState.walkWest;
        run        = controllerState.run;
        attack     = controllerState.attack;
        selectSlot = controllerState.selectSlot;
        skipFx     = false;
    }

    bool Equals(InputSample &other) {
        return (
            ownerId    == other.ownerId    &&
            walkNorth  == other.walkNorth  &&
            walkEast   == other.walkEast   &&
            walkSouth  == other.walkSouth  &&
            walkWest   == other.walkWest   &&
            run        == other.run        &&
            attack     == other.attack     &&
            selectSlot == other.selectSlot
        );
    }
};