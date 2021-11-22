#pragma once
#include "player.h"
#include "slime.h"
#include "dlb_types.h"

struct PlayerSnapshot {
    uint32_t id           {};
    uint32_t nameLength   {};
    char     name         [USERNAME_LENGTH_MAX]{};
    float    acc_x        {};
    float    acc_y        {};
    float    acc_z        {};
    float    vel_x        {};
    float    vel_y        {};
    float    vel_z        {};
    float    prev_pos_x   {};
    float    prev_pos_y   {};
    float    prev_pos_z   {};
    float    pos_x        {};
    float    pos_y        {};
    float    pos_z        {};
    float    hitPoints    {};
    float    hitPointsMax {};
};

struct SlimeSnapshot {
    uint32_t id           {};
    float    acc_x        {};
    float    acc_y        {};
    float    acc_z        {};
    float    vel_x        {};
    float    vel_y        {};
    float    vel_z        {};
    float    prev_pos_x   {};
    float    prev_pos_y   {};
    float    prev_pos_z   {};
    float    pos_x        {};
    float    pos_y        {};
    float    pos_z        {};
    float    hitPoints    {};
    float    hitPointsMax {};
    float    scale        {};
};

struct WorldSnapshot {
    uint32_t       playerId {};  // id of player this snapshot is intended for
    uint32_t       inputSeq {};  // sequence # of last processed input
    uint32_t       tick     {};
    PlayerSnapshot players  [WORLD_SNAPSHOT_PLAYERS_MAX]{};
    SlimeSnapshot  slimes   [WORLD_SNAPSHOT_ENTITIES_MAX]{};
};