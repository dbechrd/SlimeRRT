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

const char *NetServer::LOG_SRC = "NetServer";

NetServer::~NetServer()
{
    CloseSocket();
}

ErrorType NetServer::OpenSocket(unsigned short socketPort)
{
E_START
    if (zed_net_udp_socket_open_localhost(&socket, socketPort, 0) < 0) {
        const char *err = zed_net_get_error();
        E_FATAL(ErrorType::SockOpenFailed, "Failed to open socket on port %hu. Zed: %s", port, err);
    }

    assert(socket.handle);
    port = socketPort;
E_CLEAN_END
}

ErrorType NetServer::SendRaw(const NetServerClient *client, const char *data, size_t size)
{
E_START
    assert(client);
    assert(client->address.host);
    assert(data);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(size <= PACKET_SIZE_MAX);

    if (zed_net_udp_socket_send(&socket, client->address, data, (int)size) < 0) {
        const char *err = zed_net_get_error();
        E_FATAL(ErrorType::SockSendFailed, "Failed to send data. Zed: %s", err);
    }
E_CLEAN_END
}

ErrorType NetServer::SendMsg(const NetServerClient *client, const NetMessage &message)
{
E_START
    static char rawPacket[PACKET_SIZE_MAX] = {};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    E_CHECK(SendRaw(client, rawPacket, rawBytes), "Failed to send packet");
E_CLEAN_END
}

ErrorType NetServer::BroadcastRaw(const char *data, size_t size)
{
    ErrorType err_code = ErrorType::Success;

    E_INFO("BROADCAST\n  %.*s", size, data);

    // Broadcast message to all connected clients
    for (auto &kv : clients) {
        assert(kv.second.address.host);
        ErrorType code = SendRaw(&kv.second, data, size);
        if (code != ErrorType::Success) {
            TraceLog(LOG_ERROR, "[NetServer] BROADCAST\n  %.*s", size, data);
            err_code = code;
            // TODO: Handle error somehow? Retry queue.. or..? Idk.. This seems fatal. Look up why Win32 might fail
        }
    }

    return err_code;
}

ErrorType NetServer::BroadcastMsg(const NetMessage &message)
{
    static char rawPacket[PACKET_SIZE_MAX]{};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    return BroadcastRaw(rawPacket, rawBytes);
}

ErrorType NetServer::BroadcastChatMessage(const char *msg, size_t msgLength)
{
    NetMessage_ChatMessage netMsg{};
    netMsg.username = SERVER_USERNAME;
    netMsg.usernameLength = sizeof(SERVER_USERNAME) - 1;
    netMsg.message = msg;
    netMsg.messageLength = msgLength;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendWelcomeBasket(NetServerClient *client)
{
E_START
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - player list

    {
        E_INFO("Sending welcome basket to %s\n", TextFormatIP(client->address));
        NetMessage_Welcome userJoinedNotification{};
        E_CHECK(SendMsg(client, userJoinedNotification), "Failed to send welcome basket");
    }

    {
        // TODO: Save from identify packet into NetServerClient, then user client->username
        const char *username = "username";
        size_t usernameLength = strlen(username);
        const char *message = TextFormat("%.*s joined the game.", usernameLength, username);
        size_t messageLength = strlen(message);

        NetMessage_ChatMessage userJoinedNotification{};
        userJoinedNotification.username = "SERVER";
        userJoinedNotification.usernameLength = sizeof("SERVER") - 1;
        userJoinedNotification.messageLength = messageLength;
        userJoinedNotification.message = message;
        E_CHECK(BroadcastMsg(userJoinedNotification), "Failed to send join notification");
    }
E_CLEAN_END
}

void NetServer::ProcessMsg(NetServerClient *client, Packet &packet)
{
    packet.message = &NetMessage::Deserialize((uint32_t *)packet.rawBytes, sizeof(packet.rawBytes));

    switch (packet.message->type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = static_cast<NetMessage_Identify &>(*packet.message);
            client->usernameLength = MIN(identMsg.usernameLength, USERNAME_LENGTH_MAX);
            memcpy(client->username, identMsg.username, client->usernameLength);
            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = static_cast<NetMessage_ChatMessage &>(*packet.message);

            // TODO(security): Validate some session token that's not known to other people to prevent impersonation
            //assert(chatMsg.usernameLength == client->usernameLength);
            //assert(!strncmp(chatMsg.username, client->username, client->usernameLength));

            // Store chat message in chat history
            chatHistory.PushNetMessage(chatMsg);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat message
            BroadcastMsg(chatMsg);
            break;
        }
        default: {
            break;
        }
    }
}

ErrorType NetServer::Listen()
{
E_START
    assert(socket.handle);

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?
    int bytes = 0;
    do {
        zed_net_address_t sender{};
        Packet &packet = packetHistory.Alloc();
        bytes = zed_net_udp_socket_receive(&socket, &sender, CSTR0(packet.rawBytes));
        if (bytes < 0) {
            // TODO: Ignore this.. or log it? zed_net doesn't pass through any useful error messages to diagnose this.
            const char *err = zed_net_get_error();
            E_FATAL(ErrorType::SockRecvFailed, "Failed to receive network data. Zed: %s", err);
        }

        if (bytes > 0) {
            packet.srcAddress = sender;
            delete packet.message;
            memset(packet.rawBytes + bytes, 0, PACKET_SIZE_MAX - (size_t)bytes);
#if 0
            time_t t = time(NULL);
            const struct tm *utc = gmtime(&t);
            size_t timeBytes = strftime(CSTR0(packet.timestampStr), "%I:%M:%S %p", utc); // 02:15:42 PM
#endif
            NetServerClient *client = 0;
            auto kv = clients.find(sender);
            if (kv == clients.end()) {
                if (clients.size() >= NET_SERVER_CLIENTS_MAX) {
                    // Send "server full" response to sender
                    if (zed_net_udp_socket_send(&socket, sender, CSTR("SERVER FULL")) < 0) {
                        const char *err = zed_net_get_error();
                        TraceLog(LOG_ERROR, "[NetServer] Failed to send network data. Zed: %s", err);
                    }

                    TraceLog(LOG_INFO, "[NetServer] Server full, client denied.");
                    continue;
                }

                client = &clients[sender];
                client->address = sender;
            } else {
                client = &kv->second;
            }

            if (!client->last_packet_received_at) {
                SendWelcomeBasket(client);
            }
            client->last_packet_received_at = GetTime();

#if 0
            const char *senderStr = TextFormatIP(sender);
            //TraceLog(LOG_INFO, "[NetServer] RECV %s\n  %s", senderStr, packet.rawBytes);
            TraceLog(LOG_INFO, "[NetServer] Sending ACK to %s\n", senderStr);
            if (zed_net_udp_socket_send(&socket, client->address, CSTR("ACK")) < 0) {
                const char *err = zed_net_get_error();
                TraceLog(LOG_ERROR, "[NetServer] Failed to send network data. Zed: %s", err);
            }
#endif

            ProcessMsg(client, packet);
            //TraceLog(LOG_INFO, "[NetClient] RECV\n  %s said %s", senderStr, packet.rawBytes);
        }
    } while (bytes > 0);

    assert(1);
E_CLEAN_END
}

void NetServer::CloseSocket()
{
    // TODO: Notify all clients that the server is stopping

    zed_net_socket_close(&socket);
    clients.clear();
    port = 0;
}