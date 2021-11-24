#pragma once
#include "dlb_types.h"

struct PlayerControllerState {
    bool  escape     {};
    bool  screenshot {};

    // Networked input
    bool     walkNorth  {};
    bool     walkEast   {};
    bool     walkSouth  {};
    bool     walkWest   {};
    bool     run        {};
    bool     attack     {};
    uint32_t selectSlot {};  // PlayerInventorySlot

    // Freecam
    bool  cameraNorth      {};
    bool  cameraEast       {};
    bool  cameraSouth      {};
    bool  cameraWest       {};
    bool  cameraRotateCW   {};
    bool  cameraRotateCCW  {};
    bool  cameraZoomOut    {};
    float cameraSpeedDelta {};
    bool  cameraReset      {};

    // Debug
    bool  dbgFindMouseTile {};
    bool  dbgImgui         {};
    bool  dbgToggleVsync   {};
    bool  dbgChatMessage   {};
    bool  dbgSpawnSam      {};
    bool  dbgToggleFreecam {};
    bool  dbgNextRtreeRect {};

    static PlayerControllerState Query(bool ignoreMouse, bool ignoreKeyboard, bool freeCamera);
};

struct InputSample {
    //uint32_t seq        {};  // monotonic input sequence number
    uint32_t ownerId    {};  // player who generated this input
    uint32_t clientTick {};  // client tick when input was detected
    bool     walkNorth  {};
    bool     walkEast   {};
    bool     walkSouth  {};
    bool     walkWest   {};
    bool     run        {};
    bool     attack     {};
    uint32_t selectSlot {};  // PlayerInventorySlot
    bool     skipFx     {};  // oonce the input has been processed once, don't trigger FX (particles, sounds, etc.)

    void FromController(uint32_t playerId, uint32_t tick, PlayerControllerState &controllerState)
    {
        //static uint32_t nextSeqNum = 0;
        //nextSeqNum = MAX(1, nextSeqNum + 1);
        //seq        = nextSeqNum;
        ownerId    = playerId;
        clientTick = tick;
        walkNorth  = controllerState.walkNorth;
        walkEast   = controllerState.walkEast;
        walkSouth  = controllerState.walkSouth;
        walkWest   = controllerState.walkWest;
        run        = controllerState.run;
        attack     = controllerState.attack;
        selectSlot = controllerState.selectSlot;
        skipFx     = false;
    }
};