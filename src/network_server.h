#pragma once
#include "packet.h"
#include "zed_net.h"
#include <stdint.h>

#define NETWORK_SERVER_MAX_CLIENTS 4
// must be power of 2 (shift modulus ring buffer)
#define NETWORK_SERVER_MAX_PACKETS 256

typedef struct {
    zed_net_address_t address;
    // todo: double last_packet_received_at
} NetworkServerClient;

typedef struct {
    unsigned short port;
    zed_net_socket_t socket;
    size_t clientsConnected;
    NetworkServerClient clients[NETWORK_SERVER_MAX_CLIENTS];
    PacketBuffer packetHistory;
} NetworkServer;

int  network_server_init         (NetworkServer *server);
int  network_server_open_socket  (NetworkServer *server, unsigned short port);
int  network_server_receive      (NetworkServer *server);
void network_server_close_socket (NetworkServer *server);
void network_server_free         (NetworkServer *server);

#if 0
int  network_packet_history_count    (const NetworkServer *server);
int  network_packet_history_next     (const NetworkServer *server, int index);
int  network_packet_history_prev     (const NetworkServer *server, int index);
int  network_packet_history_newest   (const NetworkServer *server);
int  network_packet_history_oldest   (const NetworkServer *server);
void network_packet_history_at       (const NetworkServer *server, int index, const Packet **packet);
#endif