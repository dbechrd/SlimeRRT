#include "controller.h"

PlayerControllerState QueryPlayerController()
{
    PlayerControllerState input = { 0 };

    bool north = IsKeyDown(KEY_W);
    bool east  = IsKeyDown(KEY_D);
    bool south = IsKeyDown(KEY_S);
    bool west  = IsKeyDown(KEY_A);

    bool running = IsKeyDown(KEY_LEFT_SHIFT);
    bool attacking = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    bool selectSlot1 = IsKeyPressed(KEY_ONE);
    bool selectSlot2 = IsKeyPressed(KEY_TWO);
    bool selectSlot3 = IsKeyPressed(KEY_THREE);
    bool selectSlot4 = IsKeyPressed(KEY_FOUR);
    bool selectSlot5 = IsKeyPressed(KEY_FIVE);
    bool selectSlot6 = IsKeyPressed(KEY_SIX);

    // Clear movement flag if opposing direction is also true
    north = north && !south;
    south = south && !north;
    east = east && !west;
    west = west && !east;

    if (north || east || south || west) {
        input.moveState = running ? PlayerMoveState_Running : PlayerMoveState_Walking;
        if      (north && east) input.direction = Direction_NorthEast;
        else if (south && east) input.direction = Direction_SouthEast;
        else if (south && west) input.direction = Direction_SouthWest;
        else if (north && west) input.direction = Direction_NorthWest;
        else if (north        ) input.direction = Direction_North;
        else if (east         ) input.direction = Direction_East;
        else if (south        ) input.direction = Direction_South;
        else if (west         ) input.direction = Direction_West;
    } else {
        input.moveState = PlayerMoveState_Idle;
    }

    input.actionState = attacking ? PlayerActionState_Attacking : PlayerActionState_None;

    if      (selectSlot1) input.selectSlot = PlayerInventorySlot_1;
    else if (selectSlot2) input.selectSlot = PlayerInventorySlot_2;
    else if (selectSlot3) input.selectSlot = PlayerInventorySlot_3;
    else if (selectSlot4) input.selectSlot = PlayerInventorySlot_4;
    else if (selectSlot5) input.selectSlot = PlayerInventorySlot_5;
    else if (selectSlot6) input.selectSlot = PlayerInventorySlot_6;
    else                  input.selectSlot = PlayerInventorySlot_None;

    return input;
}