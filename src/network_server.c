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

    server->packetHistory.capacity = NETWORK_SERVER_PACKET_HISTORY_MAX;
    server->packetHistory.packets = calloc(server->packetHistory.capacity, sizeof(*server->packetHistory.packets));
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

    if (zed_net_udp_socket_open(&server->socket, port, 1) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkServer] Failed to start server on port %hu. Error: %s", server->port, err);
        return 0;
    }

    assert(server->socket.handle);
    server->port = port;
    return 1;
}

static void network_server_broadcast(NetworkServer *server, const char *data, size_t size)
{
    // Broadcast message to all connected clients
    for (int i = 0; i < NETWORK_SERVER_CLIENTS_MAX; ++i) {
        if (server->clients[i].address.host) {
            if (zed_net_udp_socket_send(&server->socket, server->clients[i].address, data, (int)size) < 0) {
                const char *err = zed_net_get_error();
                TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
            }
        }
    }
    TraceLog(LOG_INFO, "[NetworkServer] BROADCAST\n  %.*s", size, data);
}

static int network_server_send_welcome_basket(NetworkServer *server, NetworkServerClient *client)
{
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - player list

    {
        // TODO: Serialize fields in a smart way..
        NetMessageType type = NetMessageType_Welcome;
        char rawPacket[PACKET_SIZE_MAX] = { 0 };
        memcpy(rawPacket + offsetof(NetMessage, type), &type, sizeof(type));
        //memcpy(rawPacket + offsetof(NetMessage, data.identify.usernameLength), usernameLength, sizeof(usernameLength));
        //memcpy(rawPacket + offsetof(NetMessage, data.identify.username), username, usernameLength);
        size_t rawBytes = offsetof(NetMessage, type) + sizeof(type);
        assert(rawBytes < PACKET_SIZE_MAX);

        TraceLog(LOG_INFO, "[NetworkServer] Sending welcome basket to %s\n", TextFormatIP(client->address));
        if (zed_net_udp_socket_send(&server->socket, client->address, rawPacket, (int)rawBytes) < 0) {
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
            return 0;
        }
    }

    {
        // TODO: Save from identify packet into NetworkServerClient, then user client->username
        const char *username = "username";
        size_t usernameLength = strlen(username);

        const char *message = TextFormat("%.*s joined the game.", usernameLength, username);
        size_t messageLength = strlen(message);

        // TODO: Serialize fields in a smart way..
        NetMessageType type = NetMessageType_ChatMessage;
        char rawPacket[PACKET_SIZE_MAX] = { 0 };
        size_t rawBytes = 0;

#if 0
        memcpy(rawPacket + offsetof(NetMessage, type), &type, sizeof(type));
        rawBytes += sizeof(type);

        memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.usernameLength), &usernameLength, sizeof(usernameLength));
        rawBytes += sizeof(usernameLength);

        memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.username), username, MIN(usernameLength, USERNAME_LENGTH_MAX));
        rawBytes += MIN(usernameLength, USERNAME_LENGTH_MAX);

        memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.messageLength), &messageLength, sizeof(messageLength));
        rawBytes += sizeof(messageLength);

        memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.message), message, MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX));
        rawBytes += MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX);

        assert(rawBytes < PACKET_SIZE_MAX);
        network_server_broadcast(server, rawPacket, rawBytes);
#endif
    }

    return 1;
}

static void network_server_process_message(NetworkServer *server, Packet *packet)
{
    switch (packet->data.message.type) {
        case NetMessageType_ChatMessage: {
            const NetMessage_ChatMessage *netMsg = &packet->data.message.data.chatMessage;

            // Store chat message in chat history
            ChatMessage *msg = chat_history_message_alloc(&server->chatHistory);
            assert(netMsg->messageLength <= CHAT_MESSAGE_LENGTH_MAX);
            msg->messageLength = MIN(netMsg->messageLength, CHAT_MESSAGE_LENGTH_MAX);
            memcpy(msg->message, netMsg->message, msg->messageLength);

#if 0
            // TODO: Serialize fields in a smart way..
            NetMessageType type = NetMessageType_ChatMessage;
            char rawPacket[PACKET_SIZE_MAX] = { 0 };
            memcpy(rawPacket + offsetof(NetMessage, type), &type, sizeof(type));
            memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.messageLength), &msg->messageLength, sizeof(msg->messageLength));
            memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.message), msg->message, MIN(msg->messageLength, CHAT_MESSAGE_LENGTH_MAX));
            size_t rawBytes = offsetof(NetMessage, data.chatMessage.message) + msg->messageLength;
            assert(rawBytes < PACKET_SIZE_MAX);
            network_server_broadcast(server, rawPacket, rawBytes);
#endif
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
        zed_net_address_t sender = { 0 };
        size_t packetIdx = (server->packetHistory.first + server->packetHistory.count) % server->packetHistory.capacity;
        assert(packetIdx < server->packetHistory.capacity);
        Packet *packet = &server->packetHistory.packets[packetIdx];
        bytes = zed_net_udp_socket_receive(&server->socket, &sender, packet->data.raw, sizeof(packet->data) - 1);
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "[NetworkServer] Failed to receive network data. Error: %s", err);
            return 0;
        }

        if (bytes > 0) {
            packet->srcAddress = sender;
            memset(packet->data.raw + bytes, 0, PACKET_SIZE_MAX - (size_t)bytes);
            // NOTE: If packet history already full, we're overwriting the oldest packet, so count stays the same
            if (server->packetHistory.count < server->packetHistory.capacity) {
                server->packetHistory.count++;
            }

            time_t t = time(NULL);
            const struct tm *utc = gmtime(&t);
            strftime(CSTR(packet->timestampStr), "%I:%M:%S %p", utc); // 02:15:42 PM

            NetworkServerClient *client = 0;
            for (int i = 0; i < NETWORK_SERVER_CLIENTS_MAX; ++i) {
                // NOTE: Must be tightly packed array (assumes client not already connected when empty slot found)
                if (!server->clients[i].address.host) {
                    client = &server->clients[i];
                    client->address = sender;
                    break;
                } else if (server->clients[i].address.host == sender.host) {
                    client = &server->clients[i];
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
                continue;
            }

            if (!client->last_packet_received_at) {
                network_server_send_welcome_basket(server, client);
                server->clientsConnected++;
            }
            client->last_packet_received_at = GetTime();

            const char *senderStr = TextFormatIP(sender);
            TraceLog(LOG_INFO, "[NetworkServer] RECV %s\n  %s", senderStr, packet->data.raw);
#if 0
            TraceLog(LOG_INFO, "[NetworkServer] Sending ACK to %s\n", senderStr);
            if (zed_net_udp_socket_send(&server->socket, client->address, CSTR("ACK")) < 0) {
                const char *err = zed_net_get_error();
                TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Error: %s", err);
            }
#endif

            network_server_process_message(server, packet);
            TraceLog(LOG_INFO, "[NetworkClient] RECV\n  %s said %s", senderStr, packet->data.raw);
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