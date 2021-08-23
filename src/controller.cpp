#include "controller.h"

PlayerControllerState PlayerControllerState::Query()
{
    PlayerControllerState input{};
    input.walkNorth = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
    input.walkEast = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
    input.walkSouth = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
    input.walkWest = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
    input.run = IsKeyDown(KEY_LEFT_SHIFT);
    input.attack = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    input.selectSlot[(int)PlayerInventorySlot::Slot_0] = IsKeyPressed(KEY_ONE);
    input.selectSlot[(int)PlayerInventorySlot::Slot_1] = IsKeyPressed(KEY_TWO);
    input.selectSlot[(int)PlayerInventorySlot::Slot_2] = IsKeyPressed(KEY_THREE);
    input.selectSlot[(int)PlayerInventorySlot::Slot_3] = IsKeyPressed(KEY_FOUR);
    input.selectSlot[(int)PlayerInventorySlot::Slot_4] = IsKeyPressed(KEY_FIVE);
    input.selectSlot[(int)PlayerInventorySlot::Slot_5] = IsKeyPressed(KEY_SIX);
    input.selectSlot[(int)PlayerInventorySlot::Slot_6] = IsKeyPressed(KEY_SEVEN);
    return input;
}