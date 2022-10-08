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
        screenshot       = IsKeyPressed(KEY_F10);
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
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                cameraZoomDelta = GetMouseWheelMove();
            } else {
                cameraSpeedDelta = GetMouseWheelMove();
            }
            primaryPress = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            primaryHold = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        }
        if (processKeyboard) {
            walkNorth = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
            walkEast  = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
            walkSouth = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
            walkWest  = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
            run       = IsKeyDown(KEY_LEFT_SHIFT);
            if (IsKeyPressed(KEY_ONE)  ) { selectSlot = PlayerInventory::SlotId_Hotbar_0; }
            if (IsKeyPressed(KEY_TWO)  ) { selectSlot = PlayerInventory::SlotId_Hotbar_1; }
            if (IsKeyPressed(KEY_THREE)) { selectSlot = PlayerInventory::SlotId_Hotbar_2; }
            if (IsKeyPressed(KEY_FOUR) ) { selectSlot = PlayerInventory::SlotId_Hotbar_3; }
            if (IsKeyPressed(KEY_FIVE) ) { selectSlot = PlayerInventory::SlotId_Hotbar_4; }
            if (IsKeyPressed(KEY_SIX)  ) { selectSlot = PlayerInventory::SlotId_Hotbar_5; }
            if (IsKeyPressed(KEY_SEVEN)) { selectSlot = PlayerInventory::SlotId_Hotbar_6; }
            if (IsKeyPressed(KEY_EIGHT)) { selectSlot = PlayerInventory::SlotId_Hotbar_7; }
            if (IsKeyPressed(KEY_NINE))  { selectSlot = PlayerInventory::SlotId_Hotbar_8; }
            if (IsKeyPressed(KEY_ZERO))  { selectSlot = PlayerInventory::SlotId_Hotbar_9; }
            toggleInventory      = IsKeyPressed(KEY_E);
            openChatTextbox      = IsKeyPressed(KEY_T);
            openChatTextboxSlash = IsKeyPressed(KEY_SLASH);
            dbgFindMouseTile     = IsKeyDown(KEY_LEFT_ALT);
            dbgChatMessage       = IsKeyPressed(KEY_C);
            dbgTeleport          = IsKeyPressed(KEY_F5);
            dbgSpawnSam          = IsKeyPressed(KEY_KP_ENTER);
            dbgToggleFreecam     = IsKeyPressed(KEY_F);
            dbgNextRtreeRect     = IsKeyDown(KEY_N) && !IsKeyDown(KEY_K);
            dbgKillRtreeRect     = IsKeyDown(KEY_K) && !IsKeyDown(KEY_N);
            dbgImgui             = IsKeyPressed(KEY_GRAVE) && IsKeyDown(KEY_LEFT_SHIFT);
            dbgJump              = IsKeyPressed(KEY_SPACE);
            dbgToggleNetUI       = IsKeyPressed(KEY_F2);
        }
    }
}