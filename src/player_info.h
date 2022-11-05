#pragma once
#include "helpers.h"
#include <cstring>

struct PlayerInfo {
    EntityID entityId   {};
    uint32_t nameLength {};
    char     name       [USERNAME_LENGTH_MAX]{};
    uint16_t ping       {};

    void SetName(const char *playerName, uint32_t playerNameLength)
    {
        nameLength = MIN(playerNameLength, USERNAME_LENGTH_MAX);
        memcpy(name, playerName, nameLength);
    }
};