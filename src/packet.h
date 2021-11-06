#pragma once
#include "helpers.h"
#include "net_message.h"
#include "enet_zpl.h"
#include <vector>

class Packet {
public:
    ENetAddress srcAddress       {};
    ENetBuffer  rawBytes         {};
    enet_uint32 timestamp        {};
    char        timestampStr[12] {};  // hh:MM:SS AM
    NetMessage  netMessage       {};  // TODO(perf): leaked in RingBuffer, need to free this before replacing Packet!

    ~Packet();
};