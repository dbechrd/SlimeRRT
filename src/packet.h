#pragma once
#include "helpers.h"
#include "zed_net.h"

typedef enum NetMessageType {
    NetMessageType_Unknown,
    NetMessageType_Identify,
    NetMessageType_Welcome,
    NetMessageType_ChatMessage,
    NetMessageType_PlayerState,
    NetMessageType_Count
} NetMessageType;

typedef struct NetMessage_Identify {
    size_t usernameLength;
    const char *username;
    //char username[USERNAME_LENGTH_MAX];
    // TODO: Encrypt packet
    //size_t passwordLength;
    //const char *password;
} NetMessage_Identify;

typedef struct NetMessage_Welcome {
    size_t unused;
} NetMessage_Welcome;

typedef struct NetMessage_ChatMessage {
    size_t usernameLength;
    const char *username;
    //char username[USERNAME_LENGTH_MAX];
    size_t messageLength;
    const char *message;
    //char message[CHAT_MESSAGE_LENGTH_MAX];
} NetMessage_ChatMessage;

#if 0
//---------------------------------------------
// Examples
//---------------------------------------------
// fist, idle          : 00 0. ...
// fist, walking N     : 00 10 000
// sword, attacking SE : 01 11 101
//---------------------------------------------
typedef struct NetMessage_PlayerInput {
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
} NetMessage_PlayerInput;
#endif

typedef struct NetMessage {
    // TODO: sequence number
    //size_t sequenceNumber;
    NetMessageType type;
    union {
        NetMessage_Identify     identify;
        NetMessage_Welcome      welcome;
        NetMessage_ChatMessage  chatMessage;
    } data;
} NetMessage;

typedef struct Packet {
    zed_net_address_t srcAddress;
    char timestampStr[12];  // hh:MM:SS AM
    char rawBytes[PACKET_SIZE_MAX];
    NetMessage message;
} Packet;

typedef struct PacketBuffer {
    size_t first;     // index of first packet (ring buffer)
    size_t count;     // current # of packets in buffer
    size_t capacity;  // maximum # of packets in buffer
    Packet *packets;  // ring buffer of packets
} PacketBuffer;

const char *TextFormatIP(zed_net_address_t address);

