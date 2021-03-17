#pragma once
#include "zed_net.h"

#define NETWORK_SERVER_MAX_CLIENTS 4

typedef struct {
    zed_net_address_t address;
    // todo: double last_packet_received_at
} network_server_client;

typedef struct {
    unsigned short port;
    zed_net_socket_t socket;
    network_server_client clients[NETWORK_SERVER_MAX_CLIENTS];
} network_server;

int          network_init();
int          network_server_start(network_server *server, unsigned short port);
int          network_server_process_incoming(network_server *server);
const char * network_last_message(network_server *server);
void         network_server_stop(network_server *server);
void         network_shutdown();