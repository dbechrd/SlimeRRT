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
    enum class Flags : char {
        None      = 0,
        Position  = 0x01,  // world position
        Direction = 0x02,  // facing direction
        Health    = 0x08,  // current health, e.g. heal/damage
        HealthMax = 0x10,  // max health
        All = Position
            | Direction
            | Health
            | HealthMax
    };

    Flags     flags        {};
    uint32_t  id           {};
    Vector3   position     {};  // teleport, move
    Direction direction    {};  // teleport, move
    float     hitPoints    {};  // heal, damage, die
    float     hitPointsMax {};  // <no events>
};

static inline PlayerSnapshot::Flags operator|(PlayerSnapshot::Flags lhs, PlayerSnapshot::Flags rhs)
{
    return static_cast<PlayerSnapshot::Flags>(static_cast<char>(lhs) | static_cast<char>(rhs));
}

static inline PlayerSnapshot::Flags &operator|=(PlayerSnapshot::Flags &lhs, PlayerSnapshot::Flags rhs)
{
    lhs = static_cast<PlayerSnapshot::Flags>(static_cast<char>(lhs) | static_cast<char>(rhs));
    return lhs;
}

static inline bool operator&(PlayerSnapshot::Flags lhs, PlayerSnapshot::Flags rhs)
{
    return static_cast<char>(lhs) & static_cast<char>(rhs);
}

struct EnemySnapshot {
    enum class Flags : char {
        None      = 0,
        Position  = 0x01,  // world position
        Direction = 0x02,  // facing direction
        Scale     = 0x04,  // sprite scale, e.g. when slimes combine
        Health    = 0x08,  // current health, e.g. heal/damage
        HealthMax = 0x10,  // max health
        All = Position
            | Direction
            | Scale
            | Health
            | HealthMax
    };

    Flags     flags        {};
    uint32_t  id           {};
    Vector3   position     {};  // teleport, move
    Direction direction    {};  // teleport, move
    float     scale        {};  // combine
    float     hitPoints    {};  // heal, damage, die
    float     hitPointsMax {};  // <no events>
};

static inline EnemySnapshot::Flags operator|(EnemySnapshot::Flags lhs, EnemySnapshot::Flags rhs)
{
    return static_cast<EnemySnapshot::Flags>(static_cast<char>(lhs) | static_cast<char>(rhs));
}

static inline EnemySnapshot::Flags &operator|=(EnemySnapshot::Flags &lhs, EnemySnapshot::Flags rhs)
{
    lhs = static_cast<EnemySnapshot::Flags>(static_cast<char>(lhs) | static_cast<char>(rhs));
    return lhs;
}

static inline bool operator&(EnemySnapshot::Flags lhs, EnemySnapshot::Flags rhs)
{
    return static_cast<char>(lhs) & static_cast<char>(rhs);
}

struct ItemSnapshot {
   enum class Flags : char {
        None       = 0,
        Position   = 0x01,  // world position
        CatalogId  = 0x02,  // type of item
        StackCount = 0x04,  // size of item stack
        PickedUp   = 0x08,  // item gone
        All = Position
            | CatalogId
            | StackCount
            | PickedUp
    };

    Flags           flags      {};
    uint32_t        id         {};
    Vector3         position   {};  // spawn, move
    Catalog::ItemID catalogId  {};  // spawn
    uint32_t        stackCount {};  // spawn, combine (future)
    bool            pickedUp   {};  // item is gone
};

static inline ItemSnapshot::Flags operator|(ItemSnapshot::Flags lhs, ItemSnapshot::Flags rhs)
{
    return static_cast<ItemSnapshot::Flags>(static_cast<char>(lhs) | static_cast<char>(rhs));
}

static inline ItemSnapshot::Flags &operator|=(ItemSnapshot::Flags &lhs, ItemSnapshot::Flags rhs)
{
    lhs = static_cast<ItemSnapshot::Flags>(static_cast<char>(lhs) | static_cast<char>(rhs));
    return lhs;
}

static inline bool operator&(ItemSnapshot::Flags lhs, ItemSnapshot::Flags rhs)
{
    return static_cast<char>(lhs) & static_cast<char>(rhs);
}

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