#pragma once
#include "direction.h"
#include "player.h"

// TODO: Send player position, velocity, state to all clients from server (different packet/struct than above!)
struct PlayerState {
    Player::MoveState   moveState   {};
    Direction           direction   {};
    Player::ActionState actionState {};
    PlayerInventorySlot selectSlot  {};
};

struct PlayerControllerState {
    bool  escape            {};
    bool  walkNorth         {};
    bool  walkEast          {};
    bool  walkSouth         {};
    bool  walkWest          {};
    bool  cameraNorth       {};
    bool  cameraEast        {};
    bool  cameraSouth       {};
    bool  cameraWest        {};
    bool  cameraRotateCW    {};
    bool  cameraRotateCCW   {};
    bool  cameraZoomOut     {};
    float cameraSpeedDelta  {};
    bool  cameraReset       {};
    bool  run               {};
    bool  attack            {};
    bool  selectSlot[(int)PlayerInventorySlot::Count]{};
    bool  screenshot        {};
    bool  dbg_findMouseTile {};
    bool  dbg_imgui         {};
    bool  dbg_toggleVsync   {};
    bool  dbg_chatMessage   {};
    bool  dbg_toggleFreecam {};
    bool  dbg_nextRtreeRect {};

    static PlayerControllerState Query(bool ignoreMouse, bool ignoreKeyboard, bool freeCamera);
};