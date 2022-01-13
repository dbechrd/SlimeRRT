#pragma once
#include "player.h"
#include "slime.h"
#include "dlb_types.h"

#if 0
struct PlayerSnapshot {
    uint32_t  id           {};
    Vector3   position     {};
    Direction direction    {};
    float     hitPoints    {};
    float     hitPointsMax {};
};

struct SlimeSnapshot {
    uint32_t  id           {};
    Vector3   position     {};
    Direction direction    {};
    float     hitPoints    {};
    float     hitPointsMax {};
    float     scale        {};
};

struct WorldItemSnapshot {
    uint32_t  id       {};
    Vector3   position {};
};

struct WorldSnapshot {
    uint32_t          playerId     {};  // id of player this snapshot is intended for
    uint32_t          lastInputAck {};  // sequence # of last processed input
    uint32_t          tick         {};
    double            recvAt       {};  // not sent over network, auto-populated when received
    uint32_t          playerCount  {};  // players in this snapshot
    PlayerSnapshot    players      [SNAPSHOT_MAX_PLAYERS]{};
    uint32_t          slimeCount   {};  // slimes in this snapshot
    SlimeSnapshot     slimes       [SNAPSHOT_MAX_SLIMES]{};
    uint32_t          itemCount    {};  // items in this snapshot
    WorldItemSnapshot items        [SNAPSHOT_MAX_ITEMS]{};
};
#endif

struct PlayerSnapshot {
    uint32_t  id           {};
    bool      nearby       {};  // [rel] true = enter vicinity, false = despawn this entity
    bool      init         {};  // [rel] if true, all fields present for initialization (welcome basket)
    bool      spawned      {};  // [rel] respawn, teleport
    bool      attacked     {};  // [rel] attacked
    bool      moved        {};  // [rel] moved       (position change)
    bool      tookDamage   {};  // [rel] took damage (health change)
    bool      healed       {};  // [rel] healed      (health change)
    // if moved
    Vector3   position     {};
    Direction direction    {};
    // if took damage
    float     hitPoints    {};
    float     hitPointsMax {};
};

struct EnemySnapshot {
    uint32_t  id           {};
    bool      nearby       {};  // [rel] true = enter vicinity, false = despawn this entity
    bool      init         {};  // if true, all fields present for initialization (welcome basket)
    bool      spawned      {};
    bool      attacked     {};
    bool      moved        {};
    bool      resized      {};
    bool      tookDamage   {};
    bool      healed       {};
    // if moved
    Vector3   position     {};
    Direction direction    {};
    // if resized
    float     scale        {};
    // if took damage
    float     hitPoints    {};
    float     hitPointsMax {};
};

struct ItemSnapshot {
    uint32_t id       {};
    bool     nearby   {};  // [rel] true = enter vicinity, false = despawn this entity
    bool     init     {};  // [rel] if true, all fields present for initialization (welcome basket)
    bool     spawned  {};  // [rel] monster kill, chest pop, player drop
    bool     moved    {};
    // if moved
    Vector3  position {};
};

struct WorldSnapshot {
    uint32_t       lastInputAck {};  // sequence # of last processed input
    uint32_t       tick         {};  // server tick this snapshot was generated on
    uint32_t       playerCount  {};  // players in this snapshot
    PlayerSnapshot players      [SNAPSHOT_MAX_PLAYERS]{};
    uint32_t       enemyCount   {};  // slimes in this snapshot
    EnemySnapshot  enemies      [SNAPSHOT_MAX_SLIMES]{};
    uint32_t       itemCount    {};  // items in this snapshot
    ItemSnapshot   items        [SNAPSHOT_MAX_ITEMS]{};
    double         recvAt       {};  // not sent over network, auto-populated when received
};