#pragma once
#include "entities/entities.h"
#include "player.h"
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
            Flags_Position  |
            Flags_Direction |
            Flags_Health    |
            Flags_HealthMax |
            Flags_Level     ,

        // Fields to always send to the owner (the player that the client is controlling)
        Flags_Owner =
            Flags_Position  |
            Flags_Direction |
            Flags_Speed     |
            Flags_Health    |
            Flags_HealthMax |
            Flags_Level     |
            Flags_XP        ,
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

struct NpcSnapshot {
    enum Flags : uint32_t {
        Flags_None      = 0,
        Flags_Despawn   = 1 << 0,  // too far away, died, etc.
        Flags_Name      = 1 << 1,  // npc name (optional)
        Flags_Position  = 1 << 2,  // world position
        Flags_Direction = 1 << 3,  // facing direction
        Flags_Scale     = 1 << 4,  // sprite scale, e.g. when slimes combine
        Flags_Health    = 1 << 5,  // current health, e.g. heal/damage
        Flags_HealthMax = 1 << 6,  // max health
        Flags_Level     = 1 << 7,  // current level
        Flags_Spawn =
            Flags_Name      |
            Flags_Position  |
            Flags_Direction |
            Flags_Scale     |
            Flags_Health    |
            Flags_HealthMax |
            Flags_Level     ,
        //Flags_OnDespawn =
        //    Flags_Despawn   |
        //    Flags_Position  |
        //    Flags_Direction |
        //    Flags_Scale     |
        //    Flags_Health    |  // IMPORTANT: Client *needs* this to determine diedAt
        //    Flags_HealthMax ,
    };

    uint32_t  id           {};
    uint32_t  flags        {};
    NPC::Type type         {};
    uint8_t   nameLength   {};
    char      name         [ENTITY_NAME_LENGTH_MAX]{};
    Vector3   position     {};  // teleport, move
    Direction direction    {};  // teleport, move
    float     scale        {};  // combine
    float     hitPoints    {};  // heal, damage, die
    float     hitPointsMax {};  // <no events>
    uint8_t   level        {};  // spawn, level up

    static const char *FlagStr(uint32_t flags) {
        thread_local static char buf[33]{};
        buf[0] = flags & Flags_Despawn   ? 'D' : '-';
        buf[1] = flags & Flags_Name      ? 'N' : '-';
        buf[2] = flags & Flags_Position  ? 'P' : '-';
        buf[3] = flags & Flags_Direction ? 'F' : '-';
        buf[4] = flags & Flags_Scale     ? 'S' : '-';
        buf[5] = flags & Flags_Health    ? 'h' : '-';
        buf[6] = flags & Flags_HealthMax ? 'H' : '-';
        buf[7] = flags & Flags_Level     ? 'L' : '-';
        return buf;
    }
};

struct ItemSnapshot {
   enum Flags : uint32_t {
        Flags_None       = 0,
        Flags_Despawn    = 1 << 0,  // picked up / stale
        Flags_ItemUid    = 1 << 1,  // uid of item
        Flags_StackCount = 1 << 2,  // size of item stack (if 0, item is picked up entirely)
        Flags_Position   = 1 << 3,  // world position
        Flags_Spawn =
            Flags_ItemUid    |
            Flags_StackCount |
            Flags_Position   ,
    };

    uint32_t flags      {};
    uint32_t id         {};  // worldItem id
    ItemUID  itemUid    {};  // item DB uid
    uint32_t stackCount {};  // spawn, partial pickup, combine nearby stacks (future)
    Vector3  position   {};  // world position
};

struct WorldSnapshot {
    uint32_t       tick          {};  // server tick this snapshot was generated on
    double         clock         {};  // server's clock time when this snapshot was taken
    uint32_t       lastInputAck  {};  // sequence # of last processed input
    float          inputOverflow {};  // amount of next sample after lastInputAck not yet processed
    uint32_t       playerCount   {};  // players in this snapshot
    PlayerSnapshot players       [SNAPSHOT_MAX_PLAYERS]{};
    uint32_t       npcCount      {};  // enemies in this snapshot
    NpcSnapshot    npcs          [SNAPSHOT_MAX_NPCS]{};
    uint32_t       itemCount     {};  // items in this snapshot
    ItemSnapshot   items         [SNAPSHOT_MAX_ITEMS]{};
};
