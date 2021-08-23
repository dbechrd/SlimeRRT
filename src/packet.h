#pragma once
#include "helpers.h"
#include "net_message.h"
#include "zed_net.h"

struct Packet {
    zed_net_address_t srcAddress       {};
    char              timestampStr[12] {};  // hh:MM:SS AM
    char              rawBytes[PACKET_SIZE_MAX]{};
    NetMessage *      message          {};
};

struct PacketBuffer {
    size_t  first    {};  // index of first packet (ring buffer)
    size_t  count    {};  // current # of packets in buffer
    size_t  capacity {};  // maximum # of packets in buffer
    Packet *packets  {};  // ring buffer of packets
};

const char *TextFormatIP(zed_net_address_t address);

