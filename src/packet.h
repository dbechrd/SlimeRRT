#pragma once
#include "helpers.h"
#include "net_message.h"
#include "enet_zpl.h"
#include <vector>

struct Packet {
    ENetAddress srcAddress       {};
    ENetBuffer  rawBytes         {};
    char        timestampStr[12] {};  // hh:MM:SS AM
    NetMessage *netMessage       {};
};

const char *TextFormatIP(ENetAddress address);