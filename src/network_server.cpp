#include "network_server.h"
#include "bit_stream.h"
#include "helpers.h"
#include "packet.h"
#include "raylib.h"
#include "dlb_types.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

int network_server_init(NetworkServer *server)
{
    assert(server);

    server->packetHistory.capacity = NETWORK_SERVER_PACKET_HISTORY_MAX;
    server->packetHistory.packets = (Packet *)calloc(server->packetHistory.capacity, sizeof(*server->packetHistory.packets));
    if (!server->packetHistory.packets) {
        TraceLog(LOG_FATAL, "[NetworkServer] Failed to allocate packet history buffer.");
        return 0;
    }

    if (!chat_history_init(&server->chatHistory)) {
        TraceLog(LOG_FATAL, "[NetworkServer] Failed to initialize chat system.\n");
        return 0;
    }

    return 1;
}

int network_server_open_socket(NetworkServer *server, unsigned short port)
{
    assert(server);

    if (zed_net_udp_socket_open(&server->socket, port, 0) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkServer] Failed to start server on port %hu. Error: %s", server->port, err);
        return 0;
    }

    assert(server->socket.handle);
    server->port = port;
    return 1;
}

static int network_server_send(const NetworkServer *server, const NetworkServerClient *client, const char *data, size_t size)
{
    assert(client);
    assert(client->address.host);
    assert(data);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(size <= PACKET_SIZE_MAX);

    if (zed_net_udp_socket_send(&server->socket, client->address, data, (int)size) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkServer] Failed to send data. Error: %s", err);
        return 0;
    }

    return 1;
}

static void network_server_broadcast(const NetworkServer *server, const char *data, size_t size)
{
    // Broadcast message to all connected clients
    for (int i = 0; i < NETWORK_SERVER_CLIENTS_MAX; ++i) {
        if (server->clients[i].address.host) {
            if (!network_server_send(server, &server->clients[i], data, size)) {
                // TODO: Handle error somehow? Retry queue.. or..? Idk.. This seems fatal. Look up why Win32 might fail
            }
        }
    }
    TraceLog(LOG_INFO, "[NetworkServer] BROADCAST\n  %.*s", size, data);
}

static int network_server_send_welcome_basket(const NetworkServer *server, NetworkServerClient *client)
{
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - player list

    {
        NetMessage userJoinedNotification {};
        userJoinedNotification.type = NetMessageType_Welcome;

        char rawPacket[PACKET_SIZE_MAX] = {};
        BitStream writer{};
        bit_stream_writer_init(&writer, (uint32_t *)rawPacket, sizeof(rawPacket));
        size_t rawBytes = serialize_net_message(&writer, &userJoinedNotification);

        TraceLog(LOG_INFO, "[NetworkServer] Sending welcome basket to %s\n", TextFormatIP(client->address));
        network_server_send(server, client, rawPacket, rawBytes);
    }

    {
        // TODO: Save from identify packet into NetworkServerClient, then user client->username
        const char *username = "username";
        size_t usernameLength = strlen(username);
        const char *message = TextFormat("%.*s joined the game.", usernameLength, username);
        size_t messageLength = strlen(message);

        NetMessage userJoinedNotification{};
        userJoinedNotification.type = NetMessageType_ChatMessage;
        userJoinedNotification.data.chatMessage.username = "SERVER";
        userJoinedNotification.data.chatMessage.usernameLength = sizeof("SERVER") - 1;
        userJoinedNotification.data.chatMessage.messageLength = messageLength;
        userJoinedNotification.data.chatMessage.message = message;

        char rawPacket[PACKET_SIZE_MAX]{};
        BitStream writer{};
        bit_stream_writer_init(&writer, (uint32_t *)rawPacket, sizeof(rawPacket));
        size_t rawBytes = serialize_net_message(&writer, &userJoinedNotification);
        network_server_broadcast(server, rawPacket, rawBytes);
    }

    return 1;
}

static void network_server_process_message(NetworkServer *server, NetworkServerClient *client, Packet *packet)
{
    BitStream reader{};
    bit_stream_reader_init(&reader, (uint32_t *)packet->rawBytes, sizeof(packet->rawBytes));
    deserialize_net_message(&reader, &packet->message);

    switch (packet->message.type) {
        case NetMessageType_Identify: {
            client->usernameLength = MIN(packet->message.data.identify.usernameLength, USERNAME_LENGTH_MAX);
            memcpy(client->username, packet->message.data.identify.username, client->usernameLength);
            break;
        } case NetMessageType_ChatMessage: {
            assert(packet->message.data.chatMessage.usernameLength == client->usernameLength);
            assert(!strncmp(packet->message.data.chatMessage.username, client->username, client->usernameLength));

            // Store chat message in chat history
            chat_history_push_net_message(&server->chatHistory, &packet->message.data.chatMessage);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat message
            char rawPacket[PACKET_SIZE_MAX] = {};
            BitStream writer = {};
            bit_stream_writer_init(&writer, (uint32_t *)rawPacket, sizeof(rawPacket));
            size_t rawBytes = serialize_net_message(&writer, &packet->message);
            network_server_broadcast(server, rawPacket, rawBytes);
            break;
        }
        default: {
            break;
        }
    }
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
        zed_net_address_t sender{};
        size_t packetIdx = (server->packetHistory.first + server->packetHistory.count) % server->packetHistory.capacity;
        assert(packetIdx < server->packetHistory.capacity);
        Packet *packet = &server->packetHistory.packets[packetIdx];
        bytes = zed_net_udp_socket_receive(&server->socket, &sender, CSTR0(packet->rawBytes));
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "[NetworkServer] Failed to receive network data. Error: %s", err);
            return 0;
        }

        if (bytes > 0) {
            packet->srcAddress = sender;
            memset(&packet->message, 0, sizeof(packet->message));
            memset(packet->rawBytes + bytes, 0, PACKET_SIZE_MAX - (size_t)bytes);
            // NOTE: If packet history already full, we're overwriting the oldest packet, so count stays the same
            if (server->packetHistory.count < server->packetHistory.capacity) {
                server->packetHistory.count++;
            }

            time_t t = time(NULL);
            const struct tm *utc = gmtime(&t);
            size_t timeBytes = strftime(CSTR0(packet->timestampStr), "%I:%M:%S %p", utc); // 02:15:42 PM

            NetworkServerClient *client = 0;
            for (int i = 0; i < NETWORK_SERVER_CLIENTS_MAX; ++i) {
                // NOTE: Must be tightly packed array (assumes client not already connected when empty slot found)
                if (!server->clients[i].address.host) {
                    client = &server->clients[i];
                    client->address = sender;
                    break;
                } else if (server->clients[i].address.host == sender.host && server->clients[i].address.port == sender.port) {
                    client = &server->clients[i];
                    break;
                }
            }

            if (!client) {
                // Send "server full" response to sender
                if (zed_net_udp_socket_send(&server->socket, sender, CSTR("FULL")) < 0) {
                    const char *err = zed_net_get_error();
                    TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
                }

                TraceLog(LOG_INFO, "[NetworkServer] Server full, client denied.");
                continue;
            }

            if (!client->last_packet_received_at) {
                network_server_send_welcome_basket(server, client);
                server->clientsConnected++;
            }
            client->last_packet_received_at = GetTime();

            const char *senderStr = TextFormatIP(sender);
            //TraceLog(LOG_INFO, "[NetworkServer] RECV %s\n  %s", senderStr, packet->rawBytes);
#if 0
            TraceLog(LOG_INFO, "[NetworkServer] Sending ACK to %s\n", senderStr);
            if (zed_net_udp_socket_send(&server->socket, client->address, CSTR("ACK")) < 0) {
                const char *err = zed_net_get_error();
                TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
            }
#endif

            network_server_process_message(server, client, packet);
            //TraceLog(LOG_INFO, "[NetworkClient] RECV\n  %s said %s", senderStr, packet->rawBytes);
        }
    } while (bytes > 0);

    return 1;
}

void network_server_close_socket(NetworkServer *server)
{
    assert(server);

    // TODO: Notify all clients that the server is stopping

    zed_net_socket_close(&server->socket);
    //memset(&server->socket, 0, sizeof(server->socket));
    //memset(&server->clients, 0, sizeof(server->clients));
}

void network_server_free(NetworkServer *server)
{
    assert(server);

    network_server_close_socket(server);
    free(server->packetHistory.packets);
    free(server->chatHistory.messages);
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