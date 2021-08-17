#pragma once
#include "direction.h"
#include "player.h"

// TODO: Send player position, velocity, state to all clients from server (different packet/struct than above!)
struct PlayerState {
    Player::MoveState moveState;
    Direction direction;
    Player::ActionState actionState;
    PlayerInventorySlot selectSlot;
};

struct PlayerControllerState {
    bool walkNorth;
    bool walkEast;
    bool walkSouth;
    bool walkWest;
    bool run;
    bool attack;
    bool selectSlot[PlayerInventorySlot_Count];
};

void QueryPlayerController(PlayerControllerState &input);