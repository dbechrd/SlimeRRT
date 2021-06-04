#pragma once
#include "direction.h"
#include "player.h"

struct PlayerControllerState {
    PlayerMoveState moveState;
    Direction direction;
    PlayerActionState actionState;
    PlayerInventorySlot selectSlot;
};

PlayerControllerState QueryPlayerController();