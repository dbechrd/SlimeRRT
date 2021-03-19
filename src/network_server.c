#include "network_server.h"
#include "raylib.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static NetworkPacket *packet_history;
static int packet_history_next = 0;

int network_init()
{
    if (zed_net_init() < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_FATAL, "Failed to initialize network utilities. Error: %s\n", err);
        return 0;
    }

    packet_history = calloc(MAX_PACKET_HISTORY, sizeof(*packet_history));
    if (!packet_history) {
        TraceLog(LOG_FATAL, "Failed to allocate packet history buffer.\n");
        return 0;
    }

    return 1;
}

int network_server_start(NetworkServer *server, unsigned short port)
{
    assert(server);
    assert(port);

    if (zed_net_udp_socket_open(&server->socket, port, 1) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "Failed to start server on port %hu. Error: %s\n", port, err);
        return 0;
    }

    assert(server->socket.handle);
    server->port = port;
    return 1;
}

int network_server_process_incoming(NetworkServer *server)
{
    assert(server);

    if (!server->socket.handle) {
        TraceLog(LOG_FATAL, "Server socket handle invalid. Cannot processing incoming packets.\n");
        return 0;
    }

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?
    int bytes = 0;
    do {
        zed_net_address_t sender = { 0 };
        NetworkPacket *packet = &packet_history[packet_history_next];
        bytes = zed_net_udp_socket_receive(&server->socket, &sender, packet->data, sizeof(packet->data) - 1);
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "Failed to receive network data. Error: %s\n", err);
            return 0;
        }

        if (bytes > 0) {
            memset(packet->data + bytes, 0, MAX_PACKET_SIZE_BYTES - (size_t)bytes);
            packet_history_next = (packet_history_next + 1) % MAX_PACKET_HISTORY;

            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            int len = snprintf(packet->timestampStr, sizeof(packet->timestampStr), "%2d:%02d:%02d",
                tm.tm_hour, tm.tm_min, tm.tm_sec);
            assert(len < sizeof(packet->timestampStr));

            const char *host = 0;
            for (int i = 0; i < NETWORK_SERVER_MAX_CLIENTS; ++i) {
                if (!server->clients[i].address.host) {
                    server->clients[i].address = sender;
                    host = zed_net_host_to_str(server->clients[i].address.host);
                    printf("[SERVER] CONNECT %s:%u\n  %s\n", host, server->clients[i].address.port, packet->data);
                    break;
                } else if (server->clients[i].address.host == sender.host) {
                    host = zed_net_host_to_str(server->clients[i].address.host);
                    printf("[SERVER] RECEIVE %s:%u\n  %s\n", host, server->clients[i].address.port, packet->data);
                    break;
                }
            }

            if (host) {
                // Broadcast incoming message
                for (int i = 0; i < NETWORK_SERVER_MAX_CLIENTS; ++i) {
                    if (server->clients[i].address.host) { // && server->clients[i].address.host != sender.host) {
                        if (zed_net_udp_socket_send(&server->socket, server->clients[i].address, packet->data, bytes) < 0) {
                            const char *err = zed_net_get_error();
                            TraceLog(LOG_ERROR, "Failed to send network data. Error: %s\n", err);
                        }
                    }
                }
                printf("[SERVER] BROADCAST\n  %s said %s\n", host, packet->data);
            } else {
                puts("Uh oh.. new client but all of the client slots are used up.. Respond 'server full' to address.\n");
            }
            fflush(stdout);
        }
    } while (bytes > 0);

    return 1;
}

int network_packet_history_count(NetworkServer *server)
{
    return MAX_PACKET_HISTORY;
}

int network_packet_history_next(NetworkServer *server, int index)
{
    assert(server);

    int next = (index + 1) % MAX_PACKET_HISTORY;
    assert(next < MAX_PACKET_HISTORY);
    return next;
}

int network_packet_history_prev(NetworkServer *server, int index)
{
    assert(server);

    int prev = (index - 1 + MAX_PACKET_HISTORY) % MAX_PACKET_HISTORY;
    assert(prev >= 0);
    return prev;
}

int network_packet_history_newest(NetworkServer *server)
{
    assert(server);

    int newest = network_packet_history_prev(server, packet_history_next);
    return newest;
}

int network_packet_history_oldest(NetworkServer *server)
{
    assert(server);

    int index = packet_history_next % MAX_PACKET_HISTORY;
    return index;
}

const NetworkPacket *network_packet_history_at(NetworkServer *server, int index)
{
    assert(server);
    assert(index >= 0);
    assert(index < MAX_PACKET_HISTORY);

    const NetworkPacket *packet = &packet_history[index];
    if (!packet->timestampStr[0]) {
        // TODO: Make packet history / chat history have proper data/length and just check length == 0 for unused
        return 0;  // empty packet data = unused buffer
    }
    return packet;
}

void network_server_stop(NetworkServer *server)
{
    assert(server);

    zed_net_socket_close(&server->socket);
}

void network_shutdown()
{
    zed_net_shutdown();
    free(packet_history);
}