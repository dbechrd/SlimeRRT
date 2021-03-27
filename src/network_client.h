#pragma once
#include "packet.h"
#include "zed_net.h"
#include "dlb_types.h"

// must be power of 2 (shift modulus ring buffer)
#define NETWORK_CLIENT_MAX_PACKETS 256

typedef struct {
    const char *serverHostname;
    zed_net_address_t server;
    zed_net_socket_t socket;
    PacketBuffer packetHistory;
} NetworkClient;

//---------------------------------------------
// Examples
//---------------------------------------------
// fist, idle          : 00 0. ...
// fist, walking N     : 00 10 000
// sword, attacking SE : 01 11 101
//---------------------------------------------
typedef struct {
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
} Net_PlayerInput;

int  network_client_init         (NetworkClient *client);
int  network_client_open_socket  (NetworkClient *client);
int  network_client_connect      (NetworkClient *client, const char *hostname, unsigned short port);
int  network_client_send         (const NetworkClient *client, const char *data, size_t len);
int  network_client_receive       (NetworkClient *client);
void network_client_disconnect   (const NetworkClient *client);
void network_client_close_socket (const NetworkClient *client);
void network_client_free         (NetworkClient *client);
