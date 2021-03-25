#pragma once

#define MAX_PACKET_SIZE_BYTES 1024

typedef enum {
    PacketType_ChatMessage,
    PacketType_PlayerState,
    PacketType_Count
} PacketType;

typedef struct {
    char hostname[18];     // buffer size of inet_ntoa()
    char timestampStr[9];  // hh:mm:ss
    char data[MAX_PACKET_SIZE_BYTES];
} Packet;

typedef struct PacketBuffer {
    size_t first;     // index of first packet (ring buffer)
    size_t count;     // current # of packets in buffer
    size_t capacity;  // maximum # of packets in buffer
    Packet *packets;  // array of packets
} PacketBuffer;
