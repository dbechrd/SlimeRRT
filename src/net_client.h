#pragma once
#include "chat.h"
#include "packet.h"
#include "world.h"
#include "dlb_types.h"

#define NET_CLIENT_PACKET_HISTORY_MAX 256

struct NetClient {
    const char     *serverHost                    {};
    unsigned short  serverPort                    {};
    ENetHost       *client                        {};
    ENetPeer       *server                        {};
    size_t          usernameLength                {};
    char            username[USERNAME_LENGTH_MAX] {};
    size_t          passwordLength                {};
    char            password[PASSWORD_LENGTH_MAX] {};

    // TODO: Could have a packet history by message type? This would allow us
    // to only store history of important messages, and/or have different
    // buffer sizes for different types of message.
    RingBuffer<Packet> packetHistory  { NET_CLIENT_PACKET_HISTORY_MAX };
    ChatHistory        chatHistory    {};

    World *serverWorld {};

    NetClient::~NetClient();
    ErrorType OpenSocket();
    ErrorType Connect(const char *serverHost, unsigned short serverPort, const char *user, const char *password);
    ErrorType SendChatMessage(const char *message, size_t messageLength);
    ErrorType Receive();
    void Disconnect();
    void CloseSocket();

private:
    static const char *LOG_SRC;

    ErrorType Auth();
    ErrorType Send(const void *data, size_t size);
    void ProcessMsg(Packet &packet);
    const char *NetClient::ServerStateString();
};