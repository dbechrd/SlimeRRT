#pragma once
#include "bit_stream.h"
#include "controller.h"
#include "entities/entities.h"
#include "player.h"
#include "tilemap.h"
#include "world_snapshot.h"
#include "enet_zpl.h"

struct NetMessage_Identify {
    // TODO: Encrypt packet
    uint32_t usernameLength {};
    char     username       [USERNAME_LENGTH_MAX + 1]{};
    uint32_t passwordLength {};
    char     password       [PASSWORD_LENGTH_MAX + 1]{};
};

struct NetMessage_Welcome {
    uint32_t motdLength  {};
    char     motd        [MOTD_LENGTH_MAX + 1]{};  // message of the day
    uint32_t playerId    {};                       // client's assigned playerId
    uint32_t playerCount {};                       // players in game
    struct NetMessage_Welcome_Player {
        uint32_t  id           {};
        uint32_t  nameLength   {};
        char      name         [USERNAME_LENGTH_MAX + 1]{};
    } players[SV_MAX_PLAYERS]{};  // player info
};

struct NetMessage_ChatMessage {
    enum class Source {
        Unknown,
        System,
        Debug,
        Server,
        Client,
        Sam,
        Count
    };
    double   recvAt         {};
    char     timestampStr   [TIMESTAMP_LENGTH + 1]{};  // hh:MM:SS AM
    Source   source         {};
    uint32_t id             {};
    uint32_t messageLength  {};
    char     message        [CHATMSG_LENGTH_MAX + 1]{};
};

struct NetMessage_Input {
    uint32_t    sampleCount {};
    InputSample samples     [CL_INPUT_SAMPLES_MAX]{};
};

struct NetMessage_WorldChunk {
    Chunk chunk {};
};

struct NetMessage_TileUpdate {
    float worldX;
    float worldY;
    Tile tile;
};

struct NetMessage_GlobalEvent {
    enum class Type : uint32_t {
        Unknown,
        PlayerJoin,     // R player joined game
        PlayerLeave,    // R player left game
        Count
    };
    static const char *TypeString(Type type)
    {
        switch (type) {
            case Type::Unknown     : return "Unknown";
            case Type::PlayerJoin  : return "PlayerJoin";
            case Type::PlayerLeave : return "PlayerLeave";
            default: return "NetMessage_GlobalEvent::Type::???";
        }
    }
    const char *TypeString() const
    {
        return NetMessage_GlobalEvent::TypeString(type);
    }

    struct PlayerJoin {
        uint32_t playerId   {};
        uint32_t nameLength {};
        char     name       [USERNAME_LENGTH_MAX + 1]{};
    };

    struct PlayerLeave {
        uint32_t playerId {};
    };

    Type type = Type::Unknown;

    union {
        PlayerJoin  playerJoin;
        PlayerLeave playerLeave;
    } data{};
};

/*
    PLAYER_JOIN
    PLAYER_LEAVE
    PLAYER_IN_GAME  (level, xp, name, etc.)

    ITEM_DROPPED
    ITEM_PICKED_UP
    ITEM_EXISTS_ON_GROUND (up to 8 items per packet)

    CHUNK_DATA: includes array of tile entities, with coords (relative to chunk), type and data

    Minecraft
    SPAWN_ENTITY (e.g. position = chest/player(for loot/drop).ground_position, velocity = Y_UP)

    Why does Minecraft have so many instances of sending e.g. "PlaySound 7" or "PlayAnimation 12"
    instead of sending events? Or are they just the same thing as events that happen to currently
    not do anything other than play a sound/animation?
*/

struct NetMessage_NearbyEvent {
    enum class Type : uint32_t {
        Unknown,
        PlayerState,
        PlayerEquip,
        EnemyState,
        ItemState,
        Count
    };
    static const char *TypeString(Type type) {
        switch (type) {
            case Type::Unknown       : return "Unknown";
            case Type::PlayerState   : return "PlayerState";
            case Type::PlayerEquip   : return "PlayerEquip";
            case Type::EnemyState    : return "EnemyState";
            case Type::ItemState     : return "ItemState";
            default: return "NetMessage_NearbyEvent::Type::???";
        }
    }
    const char *TypeString() {
        return NetMessage_NearbyEvent::TypeString(type);
    }

    struct PlayerState {
        uint32_t  id           {};
        bool      nearby       {};  // [rel] if false, despawn this entity
        bool      spawned      {};  // [rel] respawn, teleport, enter vicinity
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

    struct EnemyState {
        uint32_t  id           {};
        bool      nearby       {};  // [rel] if false, despawn this entity
        bool      spawned      {};
        bool      attacked     {};
        bool      moved        {};
        bool      tookDamage   {};
        bool      healed       {};
        // if moved
        Vector3   position     {};
        Direction direction    {};
        // if took damage
        float     hitPoints    {};
        float     hitPointsMax {};
    };

    struct ItemState {
        uint32_t id      {};
        bool     nearby  {};  // [rel] monster kill, chest pop, player drop, enter vicinity
        bool     spawned {};
        bool     moved   {};
        // if moved
        Vector3  position {};
    };

    Type type = Type::Unknown;
    union {
        PlayerState playerState;
        EnemyState  enemyState;
        ItemState   itemState;
    } data{};
};

struct NetMessage_InventoryUpdate {
    struct SlotUpdate {
        uint8_t slotId  {};
        ItemUID itemUid {};
    };

    uint8_t    slotCount {};
    SlotUpdate slots     [CL_INVENTORY_UPDATE_SLOTS_MAX]{};
};

struct NetMessage_SlotClick {
    uint8_t slotId      {};
    uint8_t doubleClick {};
};

struct NetMessage_SlotScroll {
    uint8_t slotId  {};
    int8_t  scrollY {};
};

struct NetMessage_SlotDrop {
    uint8_t  slotId {};
    uint32_t count  {};
};

struct NetMessage_TileInteract {
    float tileX {};
    float tileY {};
};

struct NetMessage {
    enum class Type : uint32_t {
        Unknown,
        Identify,
        Welcome,
        ChatMessage,
        Input,
        WorldChunk,
        TileUpdate,
        WorldSnapshot,
        GlobalEvent,
        NearbyEvent,
        InventoryUpdate,
        SlotClick,
        SlotScroll,
        SlotDrop,
        TileInteract,
        Count
    };
    static const char *TypeString(Type type)
    {
        switch (type) {
            case Type::Unknown         : return "Unknown";
            case Type::Identify        : return "Identify";
            case Type::ChatMessage     : return "ChatMessage";
            case Type::Welcome         : return "Welcome";
            case Type::Input           : return "Input";
            case Type::WorldChunk      : return "WorldChunk";
            case Type::TileUpdate      : return "TileUpdate";
            case Type::WorldSnapshot   : return "WorldSnapshot";
            case Type::GlobalEvent     : return "GlobalEvent";
            case Type::NearbyEvent     : return "NearbyEvent";
            case Type::InventoryUpdate : return "InventoryUpdate";
            case Type::SlotClick       : return "SlotClick";
            case Type::SlotScroll      : return "SlotScroll";
            case Type::SlotDrop        : return "SlotDrop";
            case Type::TileInteract    : return "TileInteract";
            default: return "NetMessage::Type::???";
        }
    }
    const char *TypeString()
    {
        return NetMessage::TypeString(type);
    }

    uint32_t connectionToken {};
    Type type = Type::Unknown;

    union {
        NetMessage_Identify        identify;
        NetMessage_ChatMessage     chatMsg;
        NetMessage_Welcome         welcome;
        NetMessage_Input           input;
        NetMessage_WorldChunk      worldChunk;
        NetMessage_TileUpdate      tileUpdate;
        WorldSnapshot              worldSnapshot;
        NetMessage_GlobalEvent     globalEvent;
        NetMessage_NearbyEvent     nearbyEvent;
        NetMessage_InventoryUpdate inventoryUpdate;
        NetMessage_SlotClick       slotClick;
        NetMessage_SlotScroll      slotScroll;
        NetMessage_SlotDrop        slotDrop;
        NetMessage_TileInteract    tileInteract;
    } data{};

    size_t Serialize(uint8_t *buf, size_t len);
    size_t Deserialize(const uint8_t *buf, size_t len);

private:
    const char *LOG_SRC = "NetMessage";
    static ENetBuffer tempBuffer;
    size_t Process(BitStream::Mode mode, uint8_t *buf, size_t len);
};
