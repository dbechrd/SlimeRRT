#pragma once
#include "player.h"
#include "slime.h"
#include "dlb_types.h"

#if 0
struct PlayerSnapshot {
    uint32_t  type           {};
    Vector3   position     {};
    Direction direction    {};
    float     hitPoints    {};
    float     hitPointsMax {};
};

struct SlimeSnapshot {
    uint32_t  type           {};
    Vector3   position     {};
    Direction direction    {};
    float     hitPoints    {};
    float     hitPointsMax {};
    float     scale        {};
};

struct WorldItemSnapshot {
    uint32_t  type       {};
    Vector3   position {};
};

struct WorldSnapshot {
    uint32_t          playerId     {};  // type of player this snapshot is intended for
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
    enum class Flags : uint32_t {
        None      = 0,
        Despawn   = 1 << 0,  // sent when client should despawn a puppet
        Position  = 1 << 1,  // world position
        Direction = 1 << 2,  // facing direction
        Speed     = 1 << 3,  // move speed
        Health    = 1 << 4,  // current health, e.g. heal/damage
        HealthMax = 1 << 5,  // max health
        Level     = 1 << 6,
        XP        = 1 << 7,
        // TODO: PublicInventory and PrivateInventory to share visible gear without showing inv contents to other players
        Inventory = 1 << 8,  // player inventory state

        // Fields to always send for puppets (players the client doesn't control) when entering their vicinity
        PuppetSpawn =
            Position
          | Direction
          | Health
          | HealthMax
          | Level
        ,

        // Fields to always send to the owner (the player that the client is controlling)
        Owner =
            Position
          | Direction
          | Speed
          | Health
          | HealthMax
          | Level
          | XP
          | Inventory
        ,
    };

    Flags           flags        {};
    uint32_t        id           {};
    Vector3         position     {};  // teleport, move
    Direction       direction    {};  // teleport, move
    float           speed        {};  // not velocity, just move speed, only needed for owner
    float           hitPoints    {};  // heal, damage, die
    float           hitPointsMax {};  // <no events>
    uint8_t         level        {};  // join, level up
    uint32_t        xp           {};  // join, kill enemy
    PlayerInventory inventory    {};  // join, inventory update
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
    enum class Flags : uint32_t {
        None      = 0,
        Despawn   = 1 << 0,  // health = 0
        Position  = 1 << 1,  // world position
        Direction = 1 << 2,  // facing direction
        Scale     = 1 << 3,  // sprite scale, e.g. when slimes combine
        Health    = 1 << 4,  // current health, e.g. heal/damage
        HealthMax = 1 << 5,  // max health
        Level     = 1 << 6,  // current level
        All = Position
            | Direction
            | Scale
            | Health
            | HealthMax
            | Level
    };

    Flags     flags        {};
    uint32_t  id           {};
    Vector3   position     {};  // teleport, move
    Direction direction    {};  // teleport, move
    float     scale        {};  // combine
    float     hitPoints    {};  // heal, damage, die
    float     hitPointsMax {};  // <no events>
    uint8_t   level        {};  // spawn, level up
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
   enum class Flags : uint32_t {
        None       = 0,
        Despawn    = 0x01,  // picked up / stale
        Position   = 0x02,  // world position
        CatalogId  = 0x04,  // itemClass of item
        StackCount = 0x08,  // size of item stack (if 0, item is picked up entirely)
        All = Position
            | CatalogId
            | StackCount
    };

    Flags    flags      {};
    uint32_t id         {};
    Vector3  position   {};  // spawn, move
    ItemType   catalogId  {};  // spawn
    uint32_t stackCount {};  // spawn, partial pickup, combine nearby stacks (future)
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