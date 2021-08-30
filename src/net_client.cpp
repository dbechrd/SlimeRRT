#include "net_client.h"
#include "bit_stream.h"
#include "chat.h"
#include "world.h"
#include "raylib.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

const char *NetClient::LOG_SRC = "NetClient";

NetClient::~NetClient()
{
    CloseSocket();
}

ErrorType NetClient::OpenSocket()
{
E_START
    if (zed_net_udp_socket_open_localhost(&socket, 0, 1) < 0) {
        const char *err = zed_net_get_error();
        E_FATAL(ErrorType::SockOpenFailed, "Failed to open socket. Zed: %s", err);
    }
    assert(socket.handle);
E_CLEAN_END
}

ErrorType NetClient::Connect(const char *hostname, unsigned short port)
{
E_START
    if (zed_net_get_address(&server, hostname, port) < 0) {
        // TODO: Handle server connect failure gracefully (e.g. user could have typed wrong ip or port).
        const char *err = zed_net_get_error();
        E_FATAL(ErrorType::SockConnFailed, "Failed to connect to server %s:%hu. Zed: %s", hostname, port, err);
    }
    assert(server.host);
    assert(server.port);
    assert(usernameLength);
    assert(username);

    NetMessage_Identify userIdent{};
    userIdent.username = username;
    userIdent.usernameLength = usernameLength;

    char rawPacket[PACKET_SIZE_MAX] = {};
    size_t rawBytes = userIdent.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));

    if (zed_net_udp_socket_send(&socket, server, rawPacket, (int)rawBytes) < 0) {
        const char *err = zed_net_get_error();
        E_FATAL(ErrorType::SockSendFailed, "Failed to send connection request. Zed: %s", err);
    }
    serverHostname = hostname;
E_CLEAN_END
}

ErrorType NetClient::Send(const char *data, size_t size)
{
E_START
    assert(socket.handle);
    assert(data);
    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(size);
    assert(size <= PACKET_SIZE_MAX);

    if (zed_net_udp_socket_send(&socket, server, data, (int)size) < 0) {
        const char *err = zed_net_get_error();
        E_FATAL(ErrorType::SockSendFailed, "Failed to send data. Zed: %s", err);
    }
E_CLEAN_END
}

ErrorType NetClient::SendChatMessage(const char *message, size_t messageLength)
{
    assert(socket.handle);
    assert(message);

    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(messageLength);
    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    size_t messageLengthSafe = MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX);

    // If we don't have a username yet (salt, client id, etc.) then we're not connected and can't send chat messages!
    // This would be weird since if we're not connected how do we see the chat box?
    assert(usernameLength);

    NetMessage_ChatMessage chatMessage{};
    chatMessage.usernameLength = usernameLength;
    chatMessage.username = username;
    chatMessage.messageLength = messageLengthSafe;
    chatMessage.message = message;

    char rawPacket[PACKET_SIZE_MAX] = {};
    size_t rawBytes = chatMessage.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    return Send(rawPacket, rawBytes);
}

void NetClient::ProcessMsg(Packet &packet)
{
    packet.message = &NetMessage::Deserialize((uint32_t *)packet.rawBytes, sizeof(packet.rawBytes));

    switch (packet.message->type) {
        case NetMessage::Type::Welcome: {
            // TODO: Store salt sent from server instead.. handshake stuffs
            //const char *username = "user";
            //usernameLength = MIN(strlen(username), USERNAME_LENGTH_MAX);
            //memcpy(username, username, usernameLength);
            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = static_cast<NetMessage_ChatMessage &>(*packet.message);
            chatHistory.PushNetMessage(chatMsg);
            break;
        }
        default: {
            TraceLog(LOG_WARNING, "[NetClient] Unrecognized message type: %d", packet.message->type);
            break;
        }
    }
}

ErrorType NetClient::Receive()
{
E_START
    assert(socket.handle);

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?
    int bytes = 0;
    do {
        zed_net_address_t sender = {};
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

            // TODO: Refactor this out into helper function somewhere (it's also in net_server.c)
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            int len = snprintf(packet.timestampStr, sizeof(packet.timestampStr), "%02d:%02d:%02d", tm.tm_hour,
                tm.tm_min, tm.tm_sec);
            assert(len < sizeof(packet.timestampStr));

            const char *senderStr = TextFormatIP(sender);
            if (sender.host != server.host || sender.port != server.port) {
                E_INFO("STRANGER DANGER! Ingnoring unsolicited packet received from %s.\n", senderStr);
                continue;
            }

            ProcessMsg(packet);
            //TraceLog(LOG_INFO, "[NetClient] RECV\n  %s said %s", senderStr, packet.rawBytes);
        }
    } while (bytes > 0);
E_CLEAN_END
}

void NetClient::Disconnect()
{
    // TODO: Send disconnect message to server
}

void NetClient::CloseSocket()
{
    Disconnect();
    zed_net_socket_close(&socket);
}