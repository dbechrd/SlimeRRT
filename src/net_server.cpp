#include "net_server.h"
#include "bit_stream.h"
#include "helpers.h"
#include "packet.h"
#include "error.h"
#include "raylib.h"
#include "dlb_types.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>

#define SERVER_USERNAME "SERVER"
static const char *LOG_SRC = "NetServer";

NetServer g_net_server;
static const unsigned int SERVER_PORT = 4040;

ErrorType net_server_run()
{
E_START
    E_CHECK(net_server_init(&g_net_server));
    E_CHECK(net_server_open_socket(&g_net_server, SERVER_PORT));
    E_CHECK(net_server_listen(&g_net_server));
E_CLEANUP
    net_server_free(&g_net_server);
E_END
}

ErrorType net_server_init(NetServer *server)
{
E_START
    assert(server);

    server->packetHistory.capacity = NET_SERVER_PACKET_HISTORY_MAX;
    server->packetHistory.packets = (Packet *)calloc(server->packetHistory.capacity, sizeof(*server->packetHistory.packets));
    for (size_t i = 0; i < server->packetHistory.capacity; i++) {
        new(server->packetHistory.packets + i) Packet{};
    }
    E_CHECK_ALLOC(server->packetHistory.packets, "Failed to allocate packet history buffer");
    E_CHECK(server->chatHistory.Init());
E_CLEAN_END
}

ErrorType net_server_open_socket(NetServer *server, unsigned short port)
{
E_START
    assert(server);

    if (zed_net_udp_socket_open(&server->socket, port, 0) < 0) {
        const char *err = zed_net_get_error();
        E_FATAL(ErrorType::NetPortInUse, "Failed to open socket on port %hu. Zed: %s", port, err);
    }

    assert(server->socket.handle);
    server->port = port;
E_CLEAN_END
}

static ErrorType net_server_send_raw(const NetServer *server, const NetworkServerClient *client, const char *data, size_t size)
{
E_START
    assert(client);
    assert(client->address.host);
    assert(data);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(size <= PACKET_SIZE_MAX);

    if (zed_net_udp_socket_send(&server->socket, client->address, data, (int)size) < 0) {
        const char *err = zed_net_get_error();
        E_ERROR(ErrorType::SockSendFailed, "Failed to send data. Zed: %s", err);
    }
E_CLEAN_END
}

static ErrorType net_server_send(const NetServer *server, const NetworkServerClient *client, const NetMessage &message)
{
E_START
    static char rawPacket[PACKET_SIZE_MAX] = {};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    E_CHECK(net_server_send_raw(server, client, rawPacket, rawBytes));
E_CLEAN_END
}

static void net_server_broadcast_raw(const NetServer *server, const char *data, size_t size)
{
    // Broadcast message to all connected clients
    for (int i = 0; i < NET_SERVER_CLIENTS_MAX; ++i) {
        if (server->clients[i].address.host) {
            if (net_server_send_raw(server, &server->clients[i], data, size) != ErrorType::Success) {
                // TODO: Handle error somehow? Retry queue.. or..? Idk.. This seems fatal. Look up why Win32 might fail
            }
        }
    }
    TraceLog(LOG_INFO, "[NetServer] BROADCAST\n  %.*s", size, data);
}

static void net_server_broadcast(const NetServer *server, const NetMessage &message)
{
    static char rawPacket[PACKET_SIZE_MAX]{};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    net_server_broadcast_raw(server, rawPacket, rawBytes);
}

void net_server_broadcast_chat_message(const NetServer *server, const char *msg, size_t msgLength)
{
    NetMessage_ChatMessage netMsg{};
    netMsg.username = SERVER_USERNAME;
    netMsg.usernameLength = sizeof(SERVER_USERNAME) - 1;
    netMsg.message = msg;
    netMsg.messageLength = msgLength;
    net_server_broadcast(server, netMsg);
}

static int net_server_send_welcome_basket(const NetServer *server, NetworkServerClient *client)
{
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - player list

    {
        TraceLog(LOG_INFO, "[NetServer] Sending welcome basket to %s\n", TextFormatIP(client->address));
        NetMessage_Welcome userJoinedNotification{};
        net_server_send(server, client, userJoinedNotification);
    }

    {
        // TODO: Save from identify packet into NetworkServerClient, then user client->username
        const char *username = "username";
        size_t usernameLength = strlen(username);
        const char *message = TextFormat("%.*s joined the game.", usernameLength, username);
        size_t messageLength = strlen(message);

        NetMessage_ChatMessage userJoinedNotification{};
        userJoinedNotification.username = "SERVER";
        userJoinedNotification.usernameLength = sizeof("SERVER") - 1;
        userJoinedNotification.messageLength = messageLength;
        userJoinedNotification.message = message;
        net_server_broadcast(server, userJoinedNotification);
    }

    return 1;
}

static void net_server_process_message(NetServer *server, NetworkServerClient *client, Packet *packet)
{
    packet->message = &NetMessage::Deserialize((uint32_t *)packet->rawBytes, sizeof(packet->rawBytes));

    switch (packet->message->type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = static_cast<NetMessage_Identify &>(*packet->message);
            client->usernameLength = MIN(identMsg.usernameLength, USERNAME_LENGTH_MAX);
            memcpy(client->username, identMsg.username, client->usernameLength);
            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = static_cast<NetMessage_ChatMessage &>(*packet->message);

            // TODO(security): Validate some session token that's not known to other people to prevent impersonation
            //assert(chatMsg.usernameLength == client->usernameLength);
            //assert(!strncmp(chatMsg.username, client->username, client->usernameLength));

            // Store chat message in chat history
            server->chatHistory.PushNetMessage(chatMsg);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat message
            net_server_broadcast(server, chatMsg);
            break;
        }
        default: {
            break;
        }
    }
}

ErrorType net_server_listen(NetServer *server)
{
E_START
    assert(server);
    assert(server->socket.handle);

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
            E_FATAL(ErrorType::SockRecvFailed, "Failed to receive network data. Zed: %s", err);
        }

        if (bytes > 0) {
            packet->srcAddress = sender;
            memset(&packet->message, 0, sizeof(packet->message));
            memset(packet->rawBytes + bytes, 0, PACKET_SIZE_MAX - (size_t)bytes);
            // NOTE: If packet history already full, we're overwriting the oldest packet, so count stays the same
            if (server->packetHistory.count < server->packetHistory.capacity) {
                server->packetHistory.count++;
            }
#if 0
            time_t t = time(NULL);
            const struct tm *utc = gmtime(&t);
            size_t timeBytes = strftime(CSTR0(packet->timestampStr), "%I:%M:%S %p", utc); // 02:15:42 PM
#endif
            NetworkServerClient *client = 0;
            for (int i = 0; i < NET_SERVER_CLIENTS_MAX; ++i) {
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
                if (zed_net_udp_socket_send(&server->socket, sender, CSTR("SERVER FULL")) < 0) {
                    const char *err = zed_net_get_error();
                    TraceLog(LOG_ERROR, "[NetServer] Failed to send network data. Zed: %s", err);
                }

                TraceLog(LOG_INFO, "[NetServer] Server full, client denied.");
                continue;
            }

            if (!client->last_packet_received_at) {
                net_server_send_welcome_basket(server, client);
                server->clientsConnected++;
            }
            client->last_packet_received_at = GetTime();

#if 0
            const char *senderStr = TextFormatIP(sender);
            //TraceLog(LOG_INFO, "[NetServer] RECV %s\n  %s", senderStr, packet->rawBytes);
            TraceLog(LOG_INFO, "[NetServer] Sending ACK to %s\n", senderStr);
            if (zed_net_udp_socket_send(&server->socket, client->address, CSTR("ACK")) < 0) {
                const char *err = zed_net_get_error();
                TraceLog(LOG_ERROR, "[NetServer] Failed to send network data. Zed: %s", err);
            }
#endif

            net_server_process_message(server, client, packet);
            //TraceLog(LOG_INFO, "[NetClient] RECV\n  %s said %s", senderStr, packet->rawBytes);
        }
    } while (bytes > 0);

E_CLEAN_END
}

void net_server_close_socket(NetServer *server)
{
    assert(server);

    // TODO: Notify all clients that the server is stopping

    zed_net_socket_close(&server->socket);
    //memset(&server->socket, 0, sizeof(server->socket));
    //memset(&server->clients, 0, sizeof(server->clients));
}

void net_server_free(NetServer *server)
{
    assert(server);

    net_server_close_socket(server);
    free(server->packetHistory.packets);
    free(server->chatHistory.messages);
    //memset(server, 0, sizeof(*server));
}

#if 0
int network_packet_history_count(NetServer *server)
{
    assert(server);

    return MAX_PACKET_HISTORY;
}

int network_packet_history_next(NetServer *server, int index)
{
    assert(server);

    int next = (index + 1) % MAX_PACKET_HISTORY;
    assert(next < MAX_PACKET_HISTORY);
    return next;
}

int network_packet_history_prev(NetServer *server, int index)
{
    assert(server);

    int prev = (index - 1 + MAX_PACKET_HISTORY) % MAX_PACKET_HISTORY;
    assert(prev >= 0);
    return prev;
}

int network_packet_history_newest(NetServer *server)
{
    assert(server);

    int newest = network_packet_history_prev(server, packet_history_next);
    return newest;
}

int network_packet_history_oldest(NetServer *server)
{
    assert(server);

    int index = packet_history_next % MAX_PACKET_HISTORY;
    return index;
}

void network_packet_history_at(NetServer *server, int index, const Packet **packet)
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