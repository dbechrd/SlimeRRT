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
    uint32_t motdLength {};
    char     motd       [MOTD_LENGTH_MAX]{};  // message of the day
    uint32_t width      {};                   // width of map in tiles
    uint32_t height     {};                   // height of map in tiles
    uint32_t playerId   {};
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
        NetMessage_Identify    identify;
        NetMessage_ChatMessage chatMsg;
        NetMessage_Welcome     welcome;
        NetMessage_WorldChunk  worldChunk;
        WorldSnapshot          worldSnapshot;
        NetMessage_Input       input;
    } data {};

    const char *TypeString(void);
    ENetBuffer Serialize(World &world);
    void Deserialize(ENetBuffer buffer, World &world);

private:
    static ENetBuffer tempBuffer;
    void Process(BitStream::Mode mode, ENetBuffer &buffer, World &world);
};