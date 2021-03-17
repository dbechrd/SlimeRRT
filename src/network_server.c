#include "network_server.h"
#include "raylib.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_PACKET_SIZE_BYTES 1024
#define MAX_PACKET_HISTORY 10
#define MAX_CHAT_HISTORY 10

static char network_buffer[MAX_PACKET_SIZE_BYTES];
static char *packet_history; //[MAX_PACKET_HISTORY][MAX_PACKET_SIZE_BYTES];
static int packet_history_last = -1;
static const char *packet_history_most_recent;

typedef struct {
    const char *message;
    size_t len;
    double timestamp;
} chat_message;

static chat_message chat_history [MAX_CHAT_HISTORY];

int network_init()
{
    if (zed_net_init() < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_FATAL, "Failed to initialize network utilities. Error: %s\n", err);
        return 0;
    }

    packet_history = calloc(MAX_PACKET_HISTORY, sizeof(network_buffer));
    if (!packet_history) {
        TraceLog(LOG_FATAL, "Failed to allocate packet history buffer.\n");
        return 0;
    }

    return 1;
}

int network_server_start(network_server *server, unsigned short port)
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

int network_server_process_incoming(network_server *server)
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
        memset(network_buffer, 0, sizeof(network_buffer));
        bytes = zed_net_udp_socket_receive(&server->socket, &sender, network_buffer, sizeof(network_buffer) - 1);
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "Failed to receive network data. Error: %s\n", err);
            return 0;
        }

        if (bytes > 0) {
            // Save packet to history buffer
            packet_history_last = (packet_history_last + 1) % MAX_PACKET_HISTORY;
            char *hist = packet_history + (packet_history_last * MAX_PACKET_SIZE_BYTES);
            memcpy(hist, network_buffer, bytes);
            memset(hist + bytes, 0, MAX_PACKET_SIZE_BYTES - (size_t)bytes);
            packet_history_most_recent = hist;

            const char *host = 0;
            for (int i = 0; i < NETWORK_SERVER_MAX_CLIENTS; ++i) {
                if (!server->clients[i].address.host) {
                    server->clients[i].address = sender;
                    host = zed_net_host_to_str(server->clients[i].address.host);
                    printf("[SERVER] CONNECT %s:%u\n  %s\n", host, server->clients[i].address.port, network_buffer);
                    break;
                } else if (server->clients[i].address.host == sender.host) {
                    host = zed_net_host_to_str(server->clients[i].address.host);
                    printf("[SERVER] RECEIVE %s:%u\n  %s\n", host, server->clients[i].address.port, network_buffer);
                    break;
                }
            }

            if (host) {
                // Broadcast incoming message
                for (int i = 0; i < NETWORK_SERVER_MAX_CLIENTS; ++i) {
                    if (server->clients[i].address.host) { // && server->clients[i].address.host != sender.host) {
                        if (zed_net_udp_socket_send(&server->socket, server->clients[i].address, network_buffer, bytes) < 0) {
                            const char *err = zed_net_get_error();
                            TraceLog(LOG_ERROR, "Failed to send network data. Error: %s\n", err);
                        }
                    }
                }
                printf("[SERVER] BROADCAST\n  %s said %s\n", host, network_buffer);
            } else {
                puts("Uh oh.. new client but all of the client slots are used up.. Respond 'server full' to address.\n");
            }
            fflush(stdout);
        }
    } while (bytes > 0);

    return 1;
}

const char *network_last_message(network_server *server)
{
    return packet_history_most_recent;
}

void network_server_stop(network_server *server)
{
    assert(server);

    zed_net_socket_close(&server->socket);
}

void network_shutdown()
{
    zed_net_shutdown();
    free(packet_history);
}