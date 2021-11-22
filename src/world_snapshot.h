#pragma once
#include "player.h"
#include "slime.h"
#include "dlb_types.h"

struct PlayerSnapshot {
    uint32_t id           {};
    uint32_t nameLength   {};
    char     name         [USERNAME_LENGTH_MAX]{};
    Vector3  position     {};
    float    hitPoints    {};
    float    hitPointsMax {};
};

struct SlimeSnapshot {
    uint32_t id           {};
    Vector3  position     {};
    float    hitPoints    {};
    float    hitPointsMax {};
    float    scale        {};
};

struct WorldSnapshot {
    uint32_t       playerId    {};  // id of player this snapshot is intended for
    uint32_t       inputSeq    {};  // sequence # of last processed input
    uint32_t       tick        {};
    double         recvAt      {};  // not sent over network, auto-populated when received
    uint32_t       playerCount {};  // players in this snapshot
    uint32_t       slimeCount  {};  // slimes in this snapshot
    PlayerSnapshot players     [WORLD_SNAPSHOT_PLAYERS_MAX]{};
    SlimeSnapshot  slimes      [WORLD_SNAPSHOT_ENTITIES_MAX]{};
};