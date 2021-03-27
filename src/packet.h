#pragma once
#include "zed_net.h"

#define MAX_PACKET_SIZE_BYTES 1024

typedef enum {
    PacketType_ChatMessage,
    PacketType_PlayerState,
    PacketType_Count
} PacketType;

typedef struct {
    zed_net_address_t srcAddress;
    char timestampStr[9];  // hh:mm:ss
    char data[MAX_PACKET_SIZE_BYTES];
} Packet;

typedef struct PacketBuffer {
    size_t first;     // index of first packet (ring buffer)
    size_t count;     // current # of packets in buffer
    size_t capacity;  // maximum # of packets in buffer
    Packet *packets;  // array of packets
} PacketBuffer;

const char *TextFormatIP(zed_net_address_t address);