#pragma once
#include "zed_net.h"
#include "dlb_types.h"

typedef struct {
    zed_net_socket_t socket;
    zed_net_address_t server;
} network_client;

int network_client_send(network_client *client, const char *data, size_t len);