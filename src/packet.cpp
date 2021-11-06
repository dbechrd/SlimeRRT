#include "packet.h"
#include "raylib/raylib.h"
#include <cstdlib>

Packet::~Packet() {
    free(rawBytes.data);
}