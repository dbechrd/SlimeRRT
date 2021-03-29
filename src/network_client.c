#include "network_client.h"
#include "chat.h"
#include "raylib.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int network_client_send_chat_message(const NetworkClient *client, const char *message, size_t messageLength)
{
    assert(client);
    assert(client->socket.handle);
    assert(message);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);

    size_t safeLen = MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX);
#if 0
    // TODO: Serialize fields in a smart way..
    NetMessageType type = NetMessageType_ChatMessage;
    char rawPacket[PACKET_SIZE_MAX] = { 0 };
    memcpy(rawPacket + offsetof(NetMessage, type), &type, sizeof(type));
    memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.messageLength), &safeLen, sizeof(safeLen));
    memcpy(rawPacket + offsetof(NetMessage, data.chatMessage.message), message, safeLen);
    size_t rawBytes = offsetof(NetMessage, data.chatMessage.message) + safeLen;
    assert(rawBytes < PACKET_SIZE_MAX);

    return network_client_send(client, rawPacket, (int)rawBytes);
#else
    return 1;
#endif
}

int network_client_init(NetworkClient *client)
{
    client->packetHistory.capacity = NETWORK_CLIENT_PACKET_HISTORY_MAX;
    client->packetHistory.packets = calloc(client->packetHistory.capacity, sizeof(*client->packetHistory.packets));
    if (!client->packetHistory.packets) {
        TraceLog(LOG_FATAL, "[NetworkClient] Failed to allocate packet history buffer.");
        return 0;
    }

    if (!chat_history_init(&client->chatHistory)) {
        TraceLog(LOG_FATAL, "[NetworkClient] Failed to initialize chat system.\n");
        return 0;
    }

    return 1;
}

int network_client_open_socket(NetworkClient *client)
{
    assert(client);

    if (zed_net_udp_socket_open(&client->socket, 0, 1) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to open socket. Error: %s", err);
        return 0;
    }
    assert(client->socket.handle);

    return 1;
}

int network_client_connect(NetworkClient *client, const char *hostname, unsigned short port)
{
    // TODO: Display loading bar while resolving, sending and waiting for server response

    if (zed_net_get_address(&client->server, hostname, port) < 0) {
        // TODO: Handle server connect failure gracefully (e.g. user could have typed wrong ip or port).
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to connect to server %s:%hu. Error: %s", hostname, port, err);
        return 0;
    }
    assert(client->server.host);
    assert(client->server.port);

#if 1
    // TODO: Some sort of "hello, i'm here" packet? Should probably request a client id to disambiguate multiple
    // clients connecting from the same NAT address? Maybe port would do that automatically? Need to test..
    // TODO: Serialize fields in a smart way..
    NetMessageType type = NetMessageType_Identify;
    const char *username = "username";
    size_t usernameLength = strlen(username);
    char rawPacket[PACKET_SIZE_MAX] = { 0 };
    memcpy(rawPacket + offsetof(NetMessage, type), &type, sizeof(type));
    memcpy(rawPacket + offsetof(NetMessage, data.identify.usernameLength), &usernameLength, sizeof(usernameLength));
    memcpy(rawPacket + offsetof(NetMessage, data.identify.username), username, MIN(usernameLength, USERNAME_LENGTH_MAX));
    size_t rawBytes = offsetof(NetMessage, data.identify.username) + usernameLength;
    assert(rawBytes < PACKET_SIZE_MAX);

    if (zed_net_udp_socket_send(&client->socket, client->server, rawPacket, (int)rawBytes) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to send connection request. Error: %s", err);
        return 0;
    }
    client->serverHostname = hostname;
#endif

    return 1;
}

int network_client_send(const NetworkClient *client, const char *data, size_t len)
{
    assert(client);
    assert(client->socket.handle);
    assert(data);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(len <= PACKET_SIZE_MAX);

    if (zed_net_udp_socket_send(&client->socket, client->server, data, (int)len) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to send data. Error: %s", err);
        return 0;
    }

    return 1;
}

static void network_client_process_message(NetworkClient *client, Packet *packet)
{
    switch (packet->data.message.type) {
        case NetMessageType_ChatMessage: {
            const NetMessage_ChatMessage *netMsg = &packet->data.message.data.chatMessage;
            ChatMessage *msg = chat_history_message_alloc(&client->chatHistory);

            assert(netMsg->usernameLength <= USERNAME_LENGTH_MAX);
            msg->usernameLength = MIN(netMsg->usernameLength, USERNAME_LENGTH_MAX);
            memcpy(msg->username, netMsg->username, msg->usernameLength);

            assert(netMsg->messageLength <= CHAT_MESSAGE_LENGTH_MAX);
            msg->messageLength = MIN(netMsg->messageLength, CHAT_MESSAGE_LENGTH_MAX);
            memcpy(msg->message, netMsg->message, msg->messageLength);

            break;
        }
        default: {
            TraceLog(LOG_WARNING, "[NetworkClient] Unrecognized message type: %d", packet->data.message.type);
            break;
        }
    }
}

int network_client_receive(NetworkClient *client)
{
    assert(client);

    if (!client->socket.handle) {
        TraceLog(LOG_FATAL, "[NetworkClient] Client socket handle invalid. Cannot processing incoming packets.");
        return 0;
    }

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?
    int bytes = 0;
    do {
        zed_net_address_t sender = { 0 };
        size_t packetIdx = (client->packetHistory.first + client->packetHistory.count) % client->packetHistory.capacity;
        assert(packetIdx < client->packetHistory.capacity);
        Packet *packet = &client->packetHistory.packets[packetIdx];
        bytes = zed_net_udp_socket_receive(&client->socket, &sender, packet->data.raw, sizeof(packet->data) - 1);
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "[NetworkClient] Failed to receive network data. Error: %s", err);
            return 0;
        }

        if (bytes > 0) {
            packet->srcAddress = sender;
            memset(packet->data.raw + bytes, 0, PACKET_SIZE_MAX - (size_t)bytes);
            // NOTE: If packet history already full, we're overwriting the oldest packet, so count stays the same
            if (client->packetHistory.count < client->packetHistory.capacity) {
                client->packetHistory.count++;
            }

            // TODO: Refactor this out into helper function somewhere (it's also in network_server.c)
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            int len = snprintf(packet->timestampStr, sizeof(packet->timestampStr), "%02d:%02d:%02d", tm.tm_hour,
                tm.tm_min, tm.tm_sec);
            assert(len < sizeof(packet->timestampStr));

            const char *senderStr = TextFormatIP(sender);
            if (sender.host != client->server.host || sender.port != client->server.port) {
                TraceLog(LOG_INFO, "[NetworkClient] STRANGER DANGER! Ingnoring unsolicited packet received from %s.\n",
                    senderStr);
                continue;
            }

            network_client_process_message(client, packet);
            TraceLog(LOG_INFO, "[NetworkClient] RECV\n  %s said %s", senderStr, packet->data.raw);
        }
    } while (bytes > 0);

    return 1;
}

void network_client_disconnect(const NetworkClient *client)
{
    assert(client);

    // TODO: Send disconnect message to server
}

void network_client_close_socket(const NetworkClient *client)
{
    assert(client);

    network_client_disconnect(client);
    zed_net_socket_close(&client->socket);
}

void network_client_free(NetworkClient *client)
{
    network_client_close_socket(client);
    free(client->packetHistory.packets);
    free(client->chatHistory.messages);
    memset(client, 0, sizeof(*client));
}