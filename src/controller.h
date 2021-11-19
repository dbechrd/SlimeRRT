#pragma once
#include "player.h"

struct PlayerControllerState {
    bool  escape     {};
    bool  screenshot {};

    // Networked input
    bool                walkNorth  {};
    bool                walkEast   {};
    bool                walkSouth  {};
    bool                walkWest   {};
    bool                run        {};
    bool                attack     {};
    PlayerInventorySlot selectSlot {};

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
    bool  dbgToggleFreecam {};
    bool  dbgNextRtreeRect {};

    static PlayerControllerState Query(bool ignoreMouse, bool ignoreKeyboard, bool freeCamera);
};