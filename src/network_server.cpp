#include "network_server.h"
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

#define SERVER_USERNAME "SERVER"
static const char *LOG_SRC = "NetworkServer";

NetworkServer g_server;
static const unsigned int SERVER_PORT = 4040;

ErrorType network_server_thread()
{
E_START
    E_CHECK(network_server_init(&g_server));
    E_CHECK(network_server_open_socket(&g_server, SERVER_PORT));
    E_CHECK(network_server_listen(&g_server));
E_CLEANUP
    network_server_free(&g_server);
E_END
}

ErrorType network_server_init(NetworkServer *server)
{
E_START
    assert(server);

    server->packetHistory.capacity = NETWORK_SERVER_PACKET_HISTORY_MAX;
    server->packetHistory.packets = (Packet *)calloc(server->packetHistory.capacity, sizeof(*server->packetHistory.packets));
    E_CHECK_ALLOC(server->packetHistory.packets, "Failed to allocate packet history buffer");
    E_CHECK(server->chatHistory.Init());
E_CLEAN_END
}

ErrorType network_server_open_socket(NetworkServer *server, unsigned short port)
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

static ErrorType network_server_send_raw(const NetworkServer *server, const NetworkServerClient *client, const char *data, size_t size)
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

static ErrorType network_server_send(const NetworkServer *server, const NetworkServerClient *client, const NetMessage &message)
{
E_START
    static char rawPacket[PACKET_SIZE_MAX] = {};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    E_CHECK(network_server_send_raw(server, client, rawPacket, rawBytes));
E_CLEAN_END
}

static void network_server_broadcast_raw(const NetworkServer *server, const char *data, size_t size)
{
    // Broadcast message to all connected clients
    for (int i = 0; i < NETWORK_SERVER_CLIENTS_MAX; ++i) {
        if (server->clients[i].address.host) {
            if (network_server_send_raw(server, &server->clients[i], data, size) != ErrorType::Success) {
                // TODO: Handle error somehow? Retry queue.. or..? Idk.. This seems fatal. Look up why Win32 might fail
            }
        }
    }
    TraceLog(LOG_INFO, "[NetworkServer] BROADCAST\n  %.*s", size, data);
}

static void network_server_broadcast(const NetworkServer *server, const NetMessage &message)
{
    static char rawPacket[PACKET_SIZE_MAX]{};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    network_server_broadcast_raw(server, rawPacket, rawBytes);
}

void network_server_broadcast_chat_message(const NetworkServer *server, const char *msg, size_t msgLength)
{
    NetMessage_ChatMessage netMsg{};
    netMsg.m_username = SERVER_USERNAME;
    netMsg.m_usernameLength = sizeof(SERVER_USERNAME) - 1;
    netMsg.m_message = msg;
    netMsg.m_messageLength = msgLength;
    network_server_broadcast(server, netMsg);
}

static int network_server_send_welcome_basket(const NetworkServer *server, NetworkServerClient *client)
{
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - player list

    {
        TraceLog(LOG_INFO, "[NetworkServer] Sending welcome basket to %s\n", TextFormatIP(client->address));
        NetMessage_Welcome userJoinedNotification{};
        network_server_send(server, client, userJoinedNotification);
    }

    {
        // TODO: Save from identify packet into NetworkServerClient, then user client->username
        const char *username = "username";
        size_t usernameLength = strlen(username);
        const char *message = TextFormat("%.*s joined the game.", usernameLength, username);
        size_t messageLength = strlen(message);

        NetMessage_ChatMessage userJoinedNotification{};
        userJoinedNotification.m_username = "SERVER";
        userJoinedNotification.m_usernameLength = sizeof("SERVER") - 1;
        userJoinedNotification.m_messageLength = messageLength;
        userJoinedNotification.m_message = message;
        network_server_broadcast(server, userJoinedNotification);
    }

    return 1;
}

static void network_server_process_message(NetworkServer *server, NetworkServerClient *client, Packet *packet)
{
    packet->message = &NetMessage::Deserialize((uint32_t *)packet->rawBytes, sizeof(packet->rawBytes));

    switch (packet->message->m_type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = static_cast<NetMessage_Identify &>(*packet->message);
            client->usernameLength = MIN(identMsg.m_usernameLength, USERNAME_LENGTH_MAX);
            memcpy(client->username, identMsg.m_username, client->usernameLength);
            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = static_cast<NetMessage_ChatMessage &>(*packet->message);

            // TODO(security): Validate some session token that's not known to other people to prevent impersonation
            //assert(chatMsg.m_usernameLength == client->usernameLength);
            //assert(!strncmp(chatMsg.m_username, client->username, client->usernameLength));

            // Store chat message in chat history
            server->chatHistory.PushNetMessage(chatMsg);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat message
            network_server_broadcast(server, chatMsg);
            break;
        }
        default: {
            break;
        }
    }
}

ErrorType network_server_listen(NetworkServer *server)
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
                if (zed_net_udp_socket_send(&server->socket, sender, CSTR("SERVER FULL")) < 0) {
                    const char *err = zed_net_get_error();
                    TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Zed: %s", err);
                }

                TraceLog(LOG_INFO, "[NetworkServer] Server full, client denied.");
                continue;
            }

            if (!client->last_packet_received_at) {
                network_server_send_welcome_basket(server, client);
                server->clientsConnected++;
            }
            client->last_packet_received_at = GetTime();

#if 0
            const char *senderStr = TextFormatIP(sender);
            //TraceLog(LOG_INFO, "[NetworkServer] RECV %s\n  %s", senderStr, packet->rawBytes);
            TraceLog(LOG_INFO, "[NetworkServer] Sending ACK to %s\n", senderStr);
            if (zed_net_udp_socket_send(&server->socket, client->address, CSTR("ACK")) < 0) {
                const char *err = zed_net_get_error();
                TraceLog(LOG_ERROR, "[NetworkServer] Failed to send network data. Zed: %s", err);
            }
#endif

            network_server_process_message(server, client, packet);
            //TraceLog(LOG_INFO, "[NetworkClient] RECV\n  %s said %s", senderStr, packet->rawBytes);
        }
    } while (bytes > 0);

E_CLEAN_END
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