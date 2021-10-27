#include "controller.h"

PlayerControllerState PlayerControllerState::Query(bool processMouse, bool processKeyboard, bool freeCamera)
{
    PlayerControllerState input{};
    if (!processMouse && !processKeyboard) {
        return input;
    }

    // Global events
    if (processKeyboard) {
        input.escape = IsKeyPressed(KEY_ESCAPE);
        input.dbg_imgui = IsKeyPressed(KEY_GRAVE);
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
            input.dbg_toggleFreecam = IsKeyPressed(KEY_F);
        }

    // Normal gameplay events
    } else {
        if (processMouse) {
            input.attack = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        }
        if (processKeyboard) {
            input.walkNorth = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
            input.walkEast = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
            input.walkSouth = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
            input.walkWest = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
            input.run = IsKeyDown(KEY_LEFT_SHIFT);
            // TODO(dlb): What happens when player selects two inventory slots at the same time?
            input.selectSlot[(int)PlayerInventorySlot::Slot_0] = IsKeyPressed(KEY_ONE);
            input.selectSlot[(int)PlayerInventorySlot::Slot_1] = IsKeyPressed(KEY_TWO);
            input.selectSlot[(int)PlayerInventorySlot::Slot_2] = IsKeyPressed(KEY_THREE);
            input.selectSlot[(int)PlayerInventorySlot::Slot_3] = IsKeyPressed(KEY_FOUR);
            input.selectSlot[(int)PlayerInventorySlot::Slot_4] = IsKeyPressed(KEY_FIVE);
            input.selectSlot[(int)PlayerInventorySlot::Slot_5] = IsKeyPressed(KEY_SIX);
            input.selectSlot[(int)PlayerInventorySlot::Slot_6] = IsKeyPressed(KEY_SEVEN);
            input.screenshot = IsKeyPressed(KEY_F11);
            input.dbg_findMouseTile = IsKeyDown(KEY_LEFT_ALT);
            input.dbg_nextFont = IsKeyPressed(KEY_SEVEN);
            input.dbg_toggleVsync = IsKeyPressed(KEY_V);
            input.dbg_chatMessage = IsKeyPressed(KEY_C);
            input.dbg_toggleFreecam = IsKeyPressed(KEY_F);
            input.dbg_nextRtreeRect = IsKeyDown(KEY_N);
        }
    }
    return input;
}