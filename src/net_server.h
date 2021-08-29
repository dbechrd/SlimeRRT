#pragma once
#include "chat.h"
#include "error.h"
#include "packet.h"
#include "dlb_murmur3.h"
#include "zed_net.h"
#include <cstdint>
#include <unordered_map>

#define NET_SERVER_CLIENTS_MAX 4
#define NET_SERVER_PACKET_HISTORY_MAX 256

struct ZedNetAddressHash {
    std::size_t operator()(const zed_net_address_t &address) const
    {
        return (size_t)dlb_murmur3(&address.host, sizeof(address.host)) ^
              ((size_t)dlb_murmur3(&address.port, sizeof(address.port)) << 1);
    }
};

struct ZedNetAddressEqual {
    bool operator()(const zed_net_address_t &lhs, const zed_net_address_t &rhs) const
    {
        return lhs.host == rhs.host &&
               lhs.port == rhs.port;
    }
};

struct NetServerClient {
    zed_net_address_t address                       {};
    double            last_packet_received_at       {};
    size_t            usernameLength                {};
    char              username[USERNAME_LENGTH_MAX] {};
};

struct NetServer {
    unsigned short   port            {};
    zed_net_socket_t socket          {};
    std::unordered_map<zed_net_address_t, NetServerClient, ZedNetAddressHash, ZedNetAddressEqual> clients{};
    
    // TODO: Could have a packet history by message type? This would allow us
    // to only store history of important messages, and/or have different
    // buffer sizes for different types of message.
    RingBuffer<Packet> packetHistory { NET_SERVER_PACKET_HISTORY_MAX };
    ChatHistory        chatHistory   {};

    ~NetServer();
    ErrorType OpenSocket(unsigned short socketPort);
    ErrorType Listen();
    void CloseSocket();

private:
    ErrorType SendRaw(const NetServerClient *client, const char *data, size_t size);
    ErrorType SendMsg(const NetServerClient *client, const NetMessage &message);
    ErrorType BroadcastRaw(const char *data, size_t size);
    ErrorType BroadcastMsg(const NetMessage &message);
    ErrorType BroadcastChatMessage(const char *msg, size_t msgLength);
    ErrorType SendWelcomeBasket(NetServerClient *client);
    void ProcessMsg(NetServerClient *client, Packet &packet);
};