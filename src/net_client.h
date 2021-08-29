#pragma once
#include "packet.h"
#include "chat.h"
#include "zed_net.h"
#include "dlb_types.h"

#define NET_CLIENT_PACKET_HISTORY_MAX 256

struct NetClient {
    const char *       serverHostname {};
    zed_net_address_t  server         {};
    zed_net_socket_t   socket         {};
    size_t             usernameLength {};
    char               username[USERNAME_LENGTH_MAX]{};

    // TODO: Could have a packet history by message type? This would allow us
    // to only store history of important messages, and/or have different
    // buffer sizes for different types of message.
    RingBuffer<Packet> packetHistory  { NET_CLIENT_PACKET_HISTORY_MAX };
    ChatHistory        chatHistory    {};

    NetClient::~NetClient();
    ErrorType OpenSocket();
    ErrorType Connect(const char *hostname, unsigned short port);
    ErrorType SendChatMessage(const char *message, size_t messageLength);
    ErrorType Receive();
    void Disconnect();
    void CloseSocket();

private:
    ErrorType Send(const char *data, size_t size);
    void ProcessMsg(Packet &packet);
};