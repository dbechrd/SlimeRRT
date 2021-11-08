#pragma once

#if 0
//---------------------------------------------
// Examples
//---------------------------------------------
// fist, idle          : 00 0. ...
// fist, walking N     : 00 10 000
// sword, attacking SE : 01 11 101
//---------------------------------------------
struct NetMessage_PlayerInput {
    // 0       - idle       (no bits will follow)
    // 1       - moving     (running bit will follow)
    // 1 0     - walking    (direction bits will follow)
    // 1 1     - running    (direction bits will follow)
    // 1 * 000 - north
    // 1 * 001 - east
    // 1 * 010 - south
    // 1 * 011 - west
    // 1 * 100 - northeast
    // 1 * 101 - southeast
    // 1 * 110 - southwest
    // 1 * 111 - northwest
    unsigned int moving : 1;
    unsigned int running : 1;
    unsigned int direction : 3;

    // 0 - none             (no bits will follow)
    // 1 - attacking
    unsigned int attacking : 1;

    // 00 - PlayerInventorySlot_1
    // 01 - PlayerInventorySlot_2
    // 10 - PlayerInventorySlot_3
    // 11 - <unused>
    unsigned int selectSlot : 2;
};
#endif

struct Tile;
struct Slime;

struct NetMessage_Identify {
    uint32_t     usernameLength {};
    const char * username       {};
    // TODO: Encrypt packet
    uint32_t     passwordLength {};
    const char * password       {};
};

struct NetMessage_ChatMessage {
    char         timestampStr[12]{};  // hh:MM:SS AM
    uint32_t     usernameLength  {};
    const char * username        {};
    uint32_t     messageLength   {};
    const char * message         {};
};

struct NetMessage_Welcome {
    uint32_t     motdLength {};
    const char * motd       {};  // message of the day
    uint32_t     width      {};  // width of map in tiles
    uint32_t     height     {};  // height of map in tiles
};

struct NetMessage_WorldChunk {
    uint32_t   offsetX     {};
    uint32_t   offsetY     {};
    uint32_t   tilesLength {};
    Tile     * tiles       {};  // serializing
};

struct NetMessage_WorldEntities {
    uint32_t    entitiesLength {};
    Slime     * entities       {};
};

struct NetMessage {
    enum class Type : uint32_t {
        Unknown,
        Identify,
        ChatMessage,
        Welcome,
        WorldChunk,
        WorldEntities,
        Count
    };

    Type type = Type::Unknown;

    union {
        NetMessage_Identify      identify;
        NetMessage_ChatMessage   chatMsg;
        NetMessage_Welcome       welcome;
        NetMessage_WorldChunk    worldChunk;
        NetMessage_WorldEntities worldEntities;
    } data {};

    ~NetMessage();
    size_t Serialize(uint32_t *buffer, size_t bufferLength);
    size_t Deserialize(uint32_t *buffer, size_t bufferLength);

private:
    void *dataBuffer {};  // dynamic memory allocated during deserialization

    size_t Process(bool reader, uint32_t *buffer, size_t bufferLength);
};