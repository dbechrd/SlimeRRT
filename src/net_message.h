#pragma once

#include "bit_stream.h"
#include "player.h"
#include "slime.h"
#include "tilemap.h"

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
    uint32_t playerIdx  {};
};

struct NetMessage_Input {
    bool  walkNorth {};
    bool  walkEast  {};
    bool  walkSouth {};
    bool  walkWest  {};
    bool  run       {};
    bool  attack    {};
    PlayerInventorySlot selectSlot {};
};

struct NetMessage_WorldChunk {
    uint32_t offsetX     {};
    uint32_t offsetY     {};
    uint32_t tilesLength {};
    Tile     tiles       [WORLD_CHUNK_TILES_MAX]{};  // serializing
};

struct NetMessage_WorldPlayers {
    uint32_t playersLength {};
    Player   players       [SERVER_MAX_PLAYERS]{};
};

struct NetMessage_WorldEntities {
    uint32_t entitiesLength {};
    Slime    entities       [WORLD_ENTITIES_MAX]{};
};

struct NetMessage {
    enum class Type : uint32_t {
        Unknown,
        Identify,
        ChatMessage,
        Welcome,
        Input,
        WorldChunk,
        WorldPlayers,
        WorldEntities,
        Count
    };

    Type type = Type::Unknown;

    union {
        NetMessage_Identify      identify;
        NetMessage_ChatMessage   chatMsg;
        NetMessage_Welcome       welcome;
        NetMessage_Input         input;
        NetMessage_WorldChunk    worldChunk;
        NetMessage_WorldPlayers  worldPlayers;
        NetMessage_WorldEntities worldEntities;
    } data {};

    const char *TypeString(void);
    ENetBuffer Serialize(void);
    void Deserialize(ENetBuffer buffer);

private:
    static ENetBuffer tempBuffer;
    void Process(BitStream::Mode mode, ENetBuffer *buffer);
};