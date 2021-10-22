#pragma once
#include "packet.h"
#include "chat.h"
#include "dlb_types.h"

#define NET_CLIENT_PACKET_HISTORY_MAX 256

struct NetClient {
    const char *serverHostname {};
    ENetHost   *client         {};
    ENetPeer   *server         {};
    size_t      usernameLength {};
    const char *username       {};

    // TODO: Could have a packet history by message type? This would allow us
    // to only store history of important messages, and/or have different
    // buffer sizes for different types of message.
    RingBuffer<Packet> packetHistory  { NET_CLIENT_PACKET_HISTORY_MAX };
    ChatHistory        chatHistory    {};

    NetClient::~NetClient();
    ErrorType OpenSocket();
    ErrorType Connect(const char *hostname, unsigned short port);
    ErrorType Auth(const char *username, const char *password);
    ErrorType SendChatMessage(const char *message, size_t messageLength);
    ErrorType Receive();
    void Disconnect();
    void CloseSocket();

private:
    static const char *LOG_SRC;

    ErrorType Send(const char *data, size_t size);
    void ProcessMsg(Packet &packet);
};