#pragma once
#include "bit_stream.h"
#include "controller.h"
#include "player.h"
#include "slime.h"
#include "tilemap.h"

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

struct NetMessage_Input {
    uint32_t            tick       {};
    bool                walkNorth  {};
    bool                walkEast   {};
    bool                walkSouth  {};
    bool                walkWest   {};
    bool                run        {};
    bool                attack     {};
    PlayerInventorySlot selectSlot {};

    void FromController(uint32_t tick, PlayerControllerState &controllerState) {
        tick       = tick;
        walkNorth  = controllerState.walkNorth;
        walkEast   = controllerState.walkEast;
        walkSouth  = controllerState.walkSouth;
        walkWest   = controllerState.walkWest;
        run        = controllerState.run;
        attack     = controllerState.attack;
        selectSlot = controllerState.selectSlot;
    }
};

struct NetMessage_WorldChunk {
    uint32_t offsetX     {};
    uint32_t offsetY     {};
    uint32_t tilesLength {};
    // world.tiles
};

struct NetMessage_WorldPlayers {
    bool unused{};
    // world.players
};

struct NetMessage_WorldEntities {
    bool unused{};
    // world.slimes
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

    uint32_t connectionToken {};
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
    ENetBuffer Serialize(World &world);
    void Deserialize(ENetBuffer buffer, World &world);

private:
    static ENetBuffer tempBuffer;
    void Process(BitStream::Mode mode, ENetBuffer &buffer, World &world);
};