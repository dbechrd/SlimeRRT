#include "controller.h"

PlayerControllerState PlayerControllerState::Query(bool processMouse, bool processKeyboard, bool freeCamera)
{
    PlayerControllerState input{};
    if (!processMouse && !processKeyboard) {
        return input;
    }

    // Global events
    if (processKeyboard) {
        input.escape           = IsKeyPressed(KEY_ESCAPE);
        input.toggleFullscreen = IsKeyPressed(KEY_F11);
        input.screenshot       = IsKeyPressed(KEY_PRINT_SCREEN);
        input.dbgToggleVsync   = IsKeyPressed(KEY_V);
    }

    // Freecam events
    if (freeCamera) {
        if (processMouse) {
            input.cameraSpeedDelta = GetMouseWheelMove();
        }
        if (processKeyboard) {
            input.cameraNorth = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
            input.cameraEast = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
            input.cameraSouth = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
            input.cameraWest = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
            input.cameraRotateCW = IsKeyDown(KEY_E) && !IsKeyDown(KEY_Q);
            input.cameraRotateCCW = IsKeyDown(KEY_Q) && !IsKeyDown(KEY_E);
            input.cameraZoomOut = IsKeyPressed(KEY_Z);
            input.cameraReset = IsKeyPressed(KEY_R);
            input.dbgToggleFreecam = IsKeyPressed(KEY_F);
        }

    // Normal gameplay events
    } else {
        if (processMouse) {
            input.attack = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        }
        if (processKeyboard) {
            input.walkNorth = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
            input.walkEast  = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
            input.walkSouth = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
            input.walkWest  = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
            input.run       = IsKeyDown(KEY_LEFT_SHIFT);
            if (IsKeyPressed(KEY_SIX)  ) { input.selectSlot = (uint32_t)PlayerInventorySlot::Slot_6; }
            if (IsKeyPressed(KEY_FIVE) ) { input.selectSlot = (uint32_t)PlayerInventorySlot::Slot_5; }
            if (IsKeyPressed(KEY_FOUR) ) { input.selectSlot = (uint32_t)PlayerInventorySlot::Slot_4; }
            if (IsKeyPressed(KEY_THREE)) { input.selectSlot = (uint32_t)PlayerInventorySlot::Slot_3; }
            if (IsKeyPressed(KEY_TWO)  ) { input.selectSlot = (uint32_t)PlayerInventorySlot::Slot_2; }
            if (IsKeyPressed(KEY_ONE)  ) { input.selectSlot = (uint32_t)PlayerInventorySlot::Slot_1; }
            input.dbgFindMouseTile = IsKeyDown(KEY_LEFT_ALT);
            input.dbgChatMessage   = IsKeyPressed(KEY_C);
            input.dbgSpawnSam      = IsKeyPressed(KEY_KP_ENTER);
            input.dbgToggleFreecam = IsKeyPressed(KEY_F);
            input.dbgNextRtreeRect = IsKeyDown(KEY_N) && !IsKeyDown(KEY_K);
            input.dbgKillRtreeRect = IsKeyDown(KEY_K) && !IsKeyDown(KEY_N);
            input.dbgImgui         = IsKeyPressed(KEY_GRAVE);
        }
    }
    return input;
}