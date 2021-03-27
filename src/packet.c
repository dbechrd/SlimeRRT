#include "packet.h"
#include "raylib.h"

const char *TextFormatIP(zed_net_address_t address)
{
    unsigned char bytes[4] = { 0 };
    bytes[0] = address.host & 0xFF;
    bytes[1] = (address.host >> 8) & 0xFF;
    bytes[2] = (address.host >> 16) & 0xFF;
    bytes[3] = (address.host >> 24) & 0xFF;
    const char *text = TextFormat("%d.%d.%d.%d:%hu", bytes[0], bytes[1], bytes[2], bytes[3], address.port);
    return text;
}