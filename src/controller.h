#pragma once
#include "player.h"

typedef struct PlayerControllerState {
    PlayerMoveState moveState;
    Direction direction;
    PlayerActionState actionState;
    PlayerInventorySlot selectSlot;
} PlayerControllerState;

PlayerControllerState QueryPlayerController();