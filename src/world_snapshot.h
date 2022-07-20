#pragma once
#include "player.h"
#include "slime.h"
#include "dlb_types.h"

struct PlayerSnapshot {
    enum Flags : uint32_t {
        Flags_None      = 0,
        Flags_Despawn   = 1 << 0,  // sent when client should despawn a puppet
        Flags_Position  = 1 << 1,  // world position
        Flags_Direction = 1 << 2,  // facing direction
        Flags_Speed     = 1 << 3,  // move speed
        Flags_Health    = 1 << 4,  // current health, e.g. heal/damage
        Flags_HealthMax = 1 << 5,  // max health
        Flags_Level     = 1 << 6,
        Flags_XP        = 1 << 7,
        // TODO: PublicInventory and PrivateInventory to share visible gear without showing inv contents to other players
        Flags_Inventory = 1 << 8,  // player inventory state

        // Fields to always send for puppets (players the client doesn't control) when entering their vicinity
        Flags_Spawn =
            Flags_Position
          | Flags_Direction
          | Flags_Health
          | Flags_HealthMax
          | Flags_Level
        ,

        // Fields to always send to the owner (the player that the client is controlling)
        Flags_Owner =
            Flags_Position
          | Flags_Direction
          | Flags_Speed
          | Flags_Health
          | Flags_HealthMax
          | Flags_Level
          | Flags_XP
          | Flags_Inventory
        ,
    };

    uint32_t        flags        {};
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

struct EnemySnapshot {
    enum Flags : uint32_t {
        Flags_None      = 0,
        Flags_Despawn   = 1 << 0,
        Flags_Position  = 1 << 1,  // world position
        Flags_Direction = 1 << 2,  // facing direction
        Flags_Scale     = 1 << 3,  // sprite scale, e.g. when slimes combine
        Flags_Health    = 1 << 4,  // current health, e.g. heal/damage
        Flags_HealthMax = 1 << 5,  // max health
        Flags_Level     = 1 << 6,  // current level
        Flags_Spawn =
            Flags_Position
          | Flags_Direction
          | Flags_Scale
          | Flags_Health
          | Flags_HealthMax
          | Flags_Level
    };

    uint32_t  flags        {};
    uint32_t  id           {};
    Vector3   position     {};  // teleport, move
    Direction direction    {};  // teleport, move
    float     scale        {};  // combine
    float     hitPoints    {};  // heal, damage, die
    float     hitPointsMax {};  // <no events>
    uint8_t   level        {};  // spawn, level up
};

struct ItemSnapshot {
   enum Flags : uint32_t {
        Flags_None       = 0,
        Flags_Despawn    = 0x01,  // picked up / stale
        Flags_Position   = 0x02,  // world position
        Flags_CatalogId  = 0x04,  // itemClass of item
        Flags_StackCount = 0x08,  // size of item stack (if 0, item is picked up entirely)
        Flags_Spawn =
            Flags_Position
          | Flags_CatalogId
          | Flags_StackCount
    };

    uint32_t flags      {};
    uint32_t id         {};
    Vector3  position   {};  // spawn, move
    ItemType catalogId  {};  // spawn
    uint32_t stackCount {};  // spawn, partial pickup, combine nearby stacks (future)
};

struct WorldSnapshot {
    uint32_t       tick          {};  // server tick this snapshot was generated on
    double         clock         {};  // server's clock time when this snapshot was taken
    uint32_t       lastInputAck  {};  // sequence # of last processed input
    float          inputOverflow {};  // amount of next sample after lastInputAck not yet processed
    uint32_t       playerCount   {};  // players in this snapshot
    PlayerSnapshot players       [SNAPSHOT_MAX_PLAYERS]{};
    uint32_t       enemyCount    {};  // slimes in this snapshot
    EnemySnapshot  enemies       [SNAPSHOT_MAX_SLIMES]{};
    uint32_t       itemCount     {};  // items in this snapshot
    ItemSnapshot   items         [SNAPSHOT_MAX_ITEMS]{};
    double         recvAt        {};  // not sent over network, auto-populated when received
};