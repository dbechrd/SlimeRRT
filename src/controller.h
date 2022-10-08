#pragma once
#include "helpers.h"
#include "dlb_types.h"

struct PlayerControllerState {
    // Global keys
    bool escape           {};
    bool toggleFullscreen {};
    bool screenshot       {};
    bool dbgToggleVsync   {};

    // Networked input
    bool   walkNorth    {};
    bool   walkEast     {};
    bool   walkSouth    {};
    bool   walkWest     {};
    bool   run          {};
    bool   primaryPress {};
    bool   primaryHold  {};
    SlotId selectSlot   {};

    // Non-networked input
    bool toggleInventory      {};
    bool openChatTextbox      {};
    bool openChatTextboxSlash {};

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
    bool dbgToggleNetUI   {};

    void Query(bool processMouse, bool processKeyboard, bool freeCamera);
};

struct InputSample {
    uint32_t  seq        {};  // monotonic input sequence number
    uint32_t  ownerId    {};  // player who generated this input
    //uint32_t  clientTick {};  // client tick when input was detected
    float     dt         {};  // duration of command in secs (https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization)
    bool      walkNorth  {};
    bool      walkEast   {};
    bool      walkSouth  {};
    bool      walkWest   {};
    bool      run        {};
    bool      primary    {};
    SlotId    selectSlot {};  // user requesting to change active slot
    bool      skipFx     {};  // once the input has been processed once, don't trigger FX (particles, sounds, etc.)

    void FromController(uint32_t playerId, uint32_t inputSeq, double frameDt, const PlayerControllerState &controllerState)
    {
        seq        = inputSeq;
        ownerId    = playerId;
        //clientTick = tick;
        dt         = (float)frameDt;
#if CL_DEBUG_SPEEDHAX
        if (g_inputMsecHax) {
            dt = g_inputMsecHax * (1.0f / 1000.0f);
        }
#endif
        walkNorth  = controllerState.walkNorth;
        walkEast   = controllerState.walkEast;
        walkSouth  = controllerState.walkSouth;
        walkWest   = controllerState.walkWest;
        run        = controllerState.run;
        primary    = controllerState.primaryHold;
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
            primary    == other.primary    &&
            selectSlot == other.selectSlot
        );
    }
};
