#include "network_client.h"
#include "raylib.h"
#include <assert.h>

int network_client_send(network_client *client, const char *data, size_t len)
{
    assert(client);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(len <= 1024);

    if (!data || !len) {
        return 1;
    }

    if (zed_net_udp_socket_send(&client->socket, client->server, data, (int)len) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "Failed to send data. Error: %s\n", err);
        return 0;
    }
    return 1;
}
