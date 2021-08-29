#pragma once
#include "helpers.h"
#include "net_message.h"
#include "zed_net.h"
#include <vector>

struct Packet {
    zed_net_address_t srcAddress       {};
    char              timestampStr[12] {};  // hh:MM:SS AM
    char              rawBytes[PACKET_SIZE_MAX]{};
    NetMessage *      message          {};
};

const char *TextFormatIP(zed_net_address_t address);

