#include "network_server.h"
#include "helpers.h"
#include "packet.h"
#include "raylib.h"
#include "dlb_types.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int network_server_init(NetworkServer *server)
{
    assert(server);

    server->packetHistory.capacity = NETWORK_SERVER_MAX_PACKETS;
    server->packetHistory.packets = calloc(server->packetHistory.capacity, sizeof(*server->packetHistory.packets));
    if (!server->packetHistory.packets) {
        TraceLog(LOG_FATAL, "[NetworkServer] Failed to allocate packet history buffer.");
        return 0;
    }
    return 1;
}

int network_server_open_socket(NetworkServer *server, unsigned short port)
{
    assert(server);

    if (zed_net_udp_socket_open(&server->socket, port, 1) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkServer] Failed to start server on port %hu. Error: %s", server->port, err);
        return 0;
    }

    assert(server->socket.handle);
    server->port = port;
    return 1;
}

int network_server_receive(NetworkServer *server)
{
    assert(server);

    if (!server->socket.handle) {
        TraceLog(LOG_FATAL, "[NetworkServer] Server socket handle invalid. Cannot processing incoming packets.");
        return 0;
    }

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?
    int bytes = 0;
    do {
        zed_net_address_t sender = { 0 };
        size_t packetIdx = (server->packetHistory.first + server->packetHistory.count) & (NETWORK_SERVER_MAX_PACKETS - 1);
        assert(packetIdx < NETWORK_SERVER_MAX_PACKETS);
        Packet *packet = &server->packetHistory.packets[packetIdx];
        bytes = zed_net_udp_socket_receive(&server->socket, &sender, packet->data, sizeof(packet->data) - 1);
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "[NetworkServer] Failed to receive network data. Error: %s", err);
            return 0;
        }

        if (bytes > 0) {
            packet->srcAddress = sender;
            memset(packet->data + bytes, 0, MAX_PACKET_SIZE_BYTES - (size_t)bytes);
            // NOTE: If packet history already full, we're overwriting the oldest packet, so count stays the same
            if (server->packetHistory.count < server->packetHistory.capacity) {
                server->packetHistory.count++;
            }

            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            int len = snprintf(packet->timestampStr, sizeof(packet->timestampStr), "%02d:%02d:%02d", tm.tm_hour,
                tm.tm_min, tm.tm_sec);
            assert(len < sizeof(packet->timestampStr));

            NetworkServerClient *client = 0;
            for (int i = 0; i < NETWORK_SERVER_MAX_CLIENTS; ++i) {
                // NOTE: This assumes tightly packed array
                if (!server->clients[i].address.host) {
                    client = &server->clients[i];
                    client->address = sender;
                    TraceLog(LOG_INFO, "[NetworkServer] RECV %s\n  %s", TextFormatIP(client->address), packet->data);

                    TraceLog(LOG_INFO, "[NetworkServer] Sending WELCOME to %s\n", TextFormatIP(client->address));
                    if (zed_net_udp_socket_send(&server->socket, client->address, CSTR("WELCOME")) < 0) {
                        const char *err = zed_net_get_error();
                        TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
                    }

                    server->clientsConnected++;
                    break;
                } else if (server->clients[i].address.host == sender.host) {
                    client = &server->clients[i];
                    TraceLog(LOG_INFO, "[NetworkServer] RECV %s\n  %s", TextFormatIP(client->address),
                        packet->data);

                    TraceLog(LOG_INFO, "[NetworkServer] Sending ACK to %s\n", TextFormatIP(client->address));
                    if (zed_net_udp_socket_send(&server->socket, client->address, CSTR("ACK")) < 0) {
                        const char *err = zed_net_get_error();
                        TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
                    }

                    break;
                }
            }

            if (!client) {
                // Send "server full" response to client
                if (zed_net_udp_socket_send(&server->socket, sender, CSTR("FULL")) < 0) {
                    const char *err = zed_net_get_error();
                    TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
                }

                TraceLog(LOG_INFO, "[NetworkServer] Server full, client denied.");
            }

#if 0
            // Broadcast incoming message
            for (int i = 0; i < NETWORK_SERVER_MAX_CLIENTS; ++i) {
                if (server->clients[i].address.host) {
                    if (zed_net_udp_socket_send(&server->socket, server->clients[i].address, packet->data, bytes) < 0) {
                        const char *err = zed_net_get_error();
                        TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
                    }
                }
            }
            TraceLog(LOG_INFO, "[NetworkServer] BROADCAST\n  %s said %s", client->hostname, packet->data);
#endif
        }
    } while (bytes > 0);

    return 1;
}

void network_server_close_socket(NetworkServer *server)
{
    assert(server);

    // TODO: Notify all clients that the server is stopping

    zed_net_socket_close(&server->socket);
    memset(&server->socket, 0, sizeof(server->socket));
    memset(&server->clients, 0, sizeof(server->clients));
}

void network_server_free(NetworkServer *server)
{
    assert(server);

    network_server_close_socket(server);
    free(server->packetHistory.packets);
    memset(server, 0, sizeof(*server));
}

#if 0
int network_packet_history_count(NetworkServer *server)
{
    assert(server);

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

void network_packet_history_at(NetworkServer *server, int index, const Packet **packet)
{
    assert(server);
    assert(index >= 0);
    assert(index < MAX_PACKET_HISTORY);
    assert(packet);

    const NetworkPacket *pkt = &packet_history[index];
    if (pkt->timestampStr[0]) {
        // TODO: Make packet history / chat history have proper data/length and just check length == 0 for unused
        *packet = pkt;
    }
}
#endif