#pragma once

#include "controller.h"
#include "helpers.h"
#include "ring_buffer.h"

struct NetStat_Frame {
    InputSample inputSample[CL_INPUT_SAMPLES_MAX]{};
};

#define FOOOOOOOO sizeof(NetStat_Frame)

struct {
    RingBuffer<NetStat_Frame, 60> buffer {};
} g_netstat;