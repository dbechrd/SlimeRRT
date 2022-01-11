#pragma once
#include "bit_stream.h"
#include "controller.h"
#include "player.h"
#include "slime.h"
#include "tilemap.h"
#include "world_snapshot.h"
#include "enet_zpl.h"

struct World;

struct NetMessage_Identify {
    // TODO: Encrypt packet
    uint32_t usernameLength {};
    char     username       [USERNAME_LENGTH_MAX]{};
    uint32_t passwordLength {};
    char     password       [PASSWORD_LENGTH_MAX]{};
};

struct NetMessage_ChatMessage {
    double   recvAt         {};
    char     timestampStr   [TIMESTAMP_LENGTH]{};  // hh:MM:SS AM
    uint32_t usernameLength {};
    char     username       [USERNAME_LENGTH_MAX]{};
    uint32_t messageLength  {};
    char     message        [CHATMSG_LENGTH_MAX]{};
};

struct NetMessage_Welcome {
    uint32_t motdLength  {};
    char     motd        [MOTD_LENGTH_MAX]{};  // message of the day
    uint32_t width       {};                   // width of map in tiles
    uint32_t height      {};                   // height of map in tiles
    uint32_t playerId    {};                   // client's assigned playerId
    uint32_t playerCount {};                   // players in game
    struct NetMessage_Welcome_Player {
        uint32_t  id           {};
        uint32_t  nameLength   {};
        char      name         [USERNAME_LENGTH_MAX]{};
    } players[SV_MAX_PLAYERS]{};  // player info
};

struct NetMessage_Input {
    uint32_t    sampleCount {};
    InputSample samples     [CL_INPUT_SAMPLES_MAX]{};
};

struct NetMessage_WorldChunk {
    uint32_t offsetX     {};
    uint32_t offsetY     {};
    uint32_t tilesLength {};
    // world.tiles
};

struct NetMessage_GlobalEvent_PlayerJoin {
    uint32_t playerId   {};
    uint32_t nameLength {};
    char     name       [USERNAME_LENGTH_MAX]{};
};

struct NetMessage_GlobalEvent_PlayerLeave {
    uint32_t playerId {};
};

struct NetMessage_GlobalEvent {
    enum class Type : uint32_t {
        Unknown,
        PlayerJoin,     // R player joined game
        PlayerLeave,    // R player left game
        Count
    };

    Type type = Type::Unknown;

    union {
        NetMessage_GlobalEvent_PlayerJoin  playerJoin;
        NetMessage_GlobalEvent_PlayerLeave playerLeave;
    } data{};
};

struct NetMessage_NearbyEvent_PlayerSpawn {
    uint32_t  playerId     {};
    Vector3   position     {};
    Direction direction    {};
    float     hitPoints    {};
    float     hitPointsMax {};
};

struct NetMessage_NearbyEvent_PlayerDespawn {
    uint32_t playerId {};
};

struct NetMessage_NearbyEvent {
    enum class Type : uint32_t {
        Unknown,
        PlayerSpawn,    // R respawn, teleport, enter vicinity
        PlayerMove,     // U move
        PlayerAttack,   // R attacking
        PlayerEquip,    // R visible equipment change (e.g. weapon/armor/aura)
        PlayerDespawn,  // R die, teleport
        EnemySpawn,     // R spawn, enter vicinity
        EnemyMove,      // U move
        EnemyAttack,    // R attacking
        EnemyDespawn,   // R die
        ItemSpawn,      // R monster kill, chest pop, player drop, enter vicinity
        ItemMove,       // U force acted on item
        ItemDespawn,    // R picked up, item on ground too long
        Count
    };

    Type type = Type::Unknown;

    union {
        NetMessage_NearbyEvent_PlayerSpawn   playerSpawn;
        NetMessage_NearbyEvent_PlayerDespawn playerDespawn;
    } data{};
};

struct NetMessage {
    enum class Type : uint32_t {
        Unknown,
        Identify,
        Welcome,
        ChatMessage,
        Input,
        WorldChunk,
        WorldSnapshot,
        GlobalEvent,
        NearbyEvent,
        Count
    };

    uint32_t connectionToken {};
    Type type = Type::Unknown;

    union {
        NetMessage_Identify    identify;
        NetMessage_ChatMessage chatMsg;
        NetMessage_Welcome     welcome;
        NetMessage_Input       input;
        NetMessage_WorldChunk  worldChunk;
        WorldSnapshot          worldSnapshot;
        NetMessage_GlobalEvent globalEvent;
        NetMessage_NearbyEvent nearbyEvent;
    } data{};

    const char *TypeString(void);
    void Serialize(const World &world, ENetBuffer &buffer);
    void Deserialize(World &world, const ENetBuffer &buffer);

private:
    static ENetBuffer tempBuffer;
    void Process(BitStream::Mode mode, ENetBuffer &buffer, World &world);
};