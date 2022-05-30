#include "controller.h"

void PlayerControllerState::Query(bool processMouse, bool processKeyboard, bool freeCamera)
{
    escape = IsKeyPressed(KEY_ESCAPE);
    if (!processMouse && !processKeyboard) {
        return;
    }

    // Global events
    if (processKeyboard) {
        toggleFullscreen = IsKeyPressed(KEY_F11);
        screenshot       = IsKeyPressed(KEY_PRINT_SCREEN);
        dbgToggleVsync   = IsKeyPressed(KEY_V);
    }

    // Freecam events
    if (freeCamera) {
        if (processMouse) {
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                cameraZoomDelta = GetMouseWheelMove();
            } else {
                cameraSpeedDelta = GetMouseWheelMove();
            }
        }
        if (processKeyboard) {
            cameraNorth      = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
            cameraEast       = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
            cameraSouth      = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
            cameraWest       = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
            cameraRotateCW   = IsKeyDown(KEY_E) && !IsKeyDown(KEY_Q);
            cameraRotateCCW  = IsKeyDown(KEY_Q) && !IsKeyDown(KEY_E);
            cameraZoomOut    = IsKeyPressed(KEY_Z);
            cameraReset      = IsKeyPressed(KEY_R);
            dbgToggleFreecam = IsKeyPressed(KEY_F);
        }

    // Normal gameplay events
    } else {
        if (processMouse) {
            attack = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
            cameraZoomDelta = GetMouseWheelMove();
        }
        if (processKeyboard) {
            walkNorth = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
            walkEast  = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
            walkSouth = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
            walkWest  = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
            run       = IsKeyDown(KEY_LEFT_SHIFT);
            if (IsKeyPressed(KEY_ONE)  ) { selectSlot = (uint32_t)PlayerInventorySlot::Hotbar_1; }
            if (IsKeyPressed(KEY_TWO)  ) { selectSlot = (uint32_t)PlayerInventorySlot::Hotbar_2; }
            if (IsKeyPressed(KEY_THREE)) { selectSlot = (uint32_t)PlayerInventorySlot::Hotbar_3; }
            if (IsKeyPressed(KEY_FOUR) ) { selectSlot = (uint32_t)PlayerInventorySlot::Hotbar_4; }
            if (IsKeyPressed(KEY_FIVE) ) { selectSlot = (uint32_t)PlayerInventorySlot::Hotbar_5; }
            if (IsKeyPressed(KEY_SIX)  ) { selectSlot = (uint32_t)PlayerInventorySlot::Hotbar_6; }
            toggleInventory  = IsKeyPressed(KEY_I);
            dbgFindMouseTile = IsKeyDown(KEY_LEFT_ALT);
            dbgChatMessage   = IsKeyPressed(KEY_C);
            dbgTeleport      = IsKeyPressed(KEY_F5);
            dbgSpawnSam      = IsKeyPressed(KEY_KP_ENTER);
            dbgToggleFreecam = IsKeyPressed(KEY_F);
            dbgNextRtreeRect = IsKeyDown(KEY_N) && !IsKeyDown(KEY_K);
            dbgKillRtreeRect = IsKeyDown(KEY_K) && !IsKeyDown(KEY_N);
            dbgImgui         = IsKeyPressed(KEY_GRAVE);
        }
    }
}