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
    char     timestampStr   [TIMESTAMP_LENGTH]{};  // hh:MM:SS AM
    uint32_t usernameLength {};
    char     username       [USERNAME_LENGTH_MAX]{};
    uint32_t messageLength  {};
    char     message        [CHAT_MESSAGE_LENGTH_MAX]{};
};

struct NetMessage_Welcome {
    uint32_t motdLength {};
    char     motd       [MOTD_LENGTH_MAX]{};  // message of the day
    uint32_t width      {};                   // width of map in tiles
    uint32_t height     {};                   // height of map in tiles
    uint32_t playerId   {};
};

//struct NetMessage_Input {
//    InputSnapshot snapshot {};
//};

struct NetMessage_WorldChunk {
    uint32_t offsetX     {};
    uint32_t offsetY     {};
    uint32_t tilesLength {};
    // world.tiles
};

//struct NetMessage_WorldSnapshot_Player {
//    uint32_t id       {};
//    uint32_t nameLen  {};
//    char     name     [USERNAME_LENGTH_MAX]{};
//    float    pos_x    {};
//    float    pos_y    {};
//    float    pos_z    {};
//    float    hp       {};
//    float    hpMax    {};
//};
//
//struct NetMessage_WorldSnapshot_Slime {
//    uint32_t id       {};
//    float    pos_x    {};
//    float    pos_y    {};
//    float    pos_z    {};
//    float    hp       {};
//    float    hpMax    {};
//    float    scale    {};
//};

//struct NetMessage_WorldSnapshot {
//    WorldSnapshot snapshot {};
//};

struct NetMessage {
    enum class Type : uint32_t {
        Unknown,
        Identify,
        ChatMessage,
        Welcome,
        Input,
        WorldChunk,
        WorldSnapshot,
        Count
    };

    uint32_t connectionToken {};
    Type type = Type::Unknown;

    union {
        NetMessage_Identify      identify;
        NetMessage_ChatMessage   chatMsg;
        NetMessage_Welcome       welcome;
        NetMessage_WorldChunk    worldChunk;
        WorldSnapshot            worldSnapshot;
        InputSnapshot            input;
    } data {};

    const char *TypeString(void);
    ENetBuffer Serialize(World &world);
    void Deserialize(ENetBuffer buffer, World &world);

private:
    static ENetBuffer tempBuffer;
    void Process(BitStream::Mode mode, ENetBuffer &buffer, World &world);
};