#include "controller.h"

void QueryPlayerController(PlayerControllerState &input)
{
    input.walkNorth = IsKeyDown(KEY_W) && !IsKeyDown(KEY_S);
    input.walkEast  = IsKeyDown(KEY_D) && !IsKeyDown(KEY_A);
    input.walkSouth = IsKeyDown(KEY_S) && !IsKeyDown(KEY_W);
    input.walkWest  = IsKeyDown(KEY_A) && !IsKeyDown(KEY_D);
    input.run       = IsKeyDown(KEY_LEFT_SHIFT);
    input.attack    = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    input.selectSlot[0] = IsKeyPressed(KEY_ONE);
    input.selectSlot[1] = IsKeyPressed(KEY_TWO);
    input.selectSlot[2] = IsKeyPressed(KEY_THREE);
    input.selectSlot[3] = IsKeyPressed(KEY_FOUR);
    input.selectSlot[4] = IsKeyPressed(KEY_FIVE);
    input.selectSlot[5] = IsKeyPressed(KEY_SIX);
}