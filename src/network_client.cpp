#include "network_client.h"
#include "bit_stream.h"
#include "chat.h"
#include "world.h"
#include "raylib.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

int network_client_init(NetworkClient *client)
{
    client->packetHistory.capacity = NETWORK_CLIENT_PACKET_HISTORY_MAX;
    client->packetHistory.packets = (Packet *)calloc(client->packetHistory.capacity, sizeof(*client->packetHistory.packets));
    if (!client->packetHistory.packets) {
        TraceLog(LOG_FATAL, "[NetworkClient] Failed to allocate packet history buffer.");
        return 0;
    }

    if (client->chatHistory.Init() != ErrorType::Success) {
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
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to open socket. Zed: %s", err);
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
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to connect to server %s:%hu. Zed: %s", hostname, port, err);
        return 0;
    }
    assert(client->server.host);
    assert(client->server.port);
    assert(client->usernameLength);
    assert(client->username);

    NetMessage_Identify userIdent{};
    userIdent.m_username = client->username;
    userIdent.m_usernameLength = client->usernameLength;

    char rawPacket[PACKET_SIZE_MAX] = {};
    size_t rawBytes = userIdent.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));

    if (zed_net_udp_socket_send(&client->socket, client->server, rawPacket, (int)rawBytes) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to send connection request. Zed: %s", err);
        return 0;
    }
    client->serverHostname = hostname;

    return 1;
}

int network_client_send(const NetworkClient *client, const char *data, size_t size)
{
    assert(client);
    assert(client->socket.handle);
    assert(data);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(size <= PACKET_SIZE_MAX);

    if (zed_net_udp_socket_send(&client->socket, client->server, data, (int)size) < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_ERROR, "[NetworkClient] Failed to send data. Zed: %s", err);
        return 0;
    }

    return 1;
}

int network_client_send_chat_message(const NetworkClient *client, const char *message, size_t messageLength)
{
    assert(client);
    assert(client->socket.handle);
    assert(message);

    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    size_t messageLengthSafe = MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX);

    // If we don't have a username yet (salt, client id, etc.) then we're not connected and can't send chat messages!
    // This would be weird since if we're not connected how do we see the chat box?
    assert(client->usernameLength);

    NetMessage_ChatMessage chatMessage{};
    chatMessage.m_usernameLength = client->usernameLength;
    chatMessage.m_username = client->username;
    chatMessage.m_messageLength = messageLengthSafe;
    chatMessage.m_message = message;

    char rawPacket[PACKET_SIZE_MAX] = {};
    size_t rawBytes = chatMessage.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    return network_client_send(client, rawPacket, rawBytes);
}

static void network_client_process_message(NetworkClient *client, Packet *packet)
{
    packet->message = &NetMessage::Deserialize((uint32_t *)packet->rawBytes, sizeof(packet->rawBytes));

    switch (packet->message->m_type) {
        case NetMessage::Type::Welcome: {
            // TODO: Store salt sent from server instead.. handshake stuffs
            //const char *username = "user";
            //client->usernameLength = MIN(strlen(username), USERNAME_LENGTH_MAX);
            //memcpy(client->username, username, client->usernameLength);
            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = static_cast<NetMessage_ChatMessage &>(*packet->message);
            client->chatHistory.PushNetMessage(chatMsg);
            break;
        }
        default: {
            TraceLog(LOG_WARNING, "[NetworkClient] Unrecognized message type: %d", packet->message->m_type);
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
        zed_net_address_t sender = {};
        size_t packetIdx = (client->packetHistory.first + client->packetHistory.count) % client->packetHistory.capacity;
        assert(packetIdx < client->packetHistory.capacity);
        Packet *packet = &client->packetHistory.packets[packetIdx];
        bytes = zed_net_udp_socket_receive(&client->socket, &sender, CSTR0(packet->rawBytes));
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            TraceLog(LOG_ERROR, "[NetworkClient] Failed to receive network data. Zed: %s", err);
            return 0;
        }

        if (bytes > 0) {
            packet->srcAddress = sender;
            memset(&packet->message, 0, sizeof(packet->message));
            memset(packet->rawBytes + bytes, 0, PACKET_SIZE_MAX - (size_t)bytes);

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
            //TraceLog(LOG_INFO, "[NetworkClient] RECV\n  %s said %s", senderStr, packet->rawBytes);
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