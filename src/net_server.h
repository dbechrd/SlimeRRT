#pragma once
#include "chat.h"
#include "error.h"
#include "packet.h"
#include "dlb_murmur3.h"
#include <cstdint>
#include <unordered_map>

#define NET_SERVER_CLIENTS_MAX 4
#define NET_SERVER_PACKET_HISTORY_MAX 256

struct NetAddressHash {
    std::size_t operator()(const ENetAddress &address) const
    {
        return (size_t)dlb_murmur3(&address.host, sizeof(address.host)) ^
              ((size_t)dlb_murmur3(&address.port, sizeof(address.port)) << 1);
    }
};

struct NetAddressEqual {
    bool operator()(const ENetAddress &lhs, const ENetAddress &rhs) const
    {
        return lhs.host.u.Word[0] == rhs.host.u.Word[0] &&
               lhs.host.u.Word[1] == rhs.host.u.Word[1] &&
               lhs.host.u.Word[2] == rhs.host.u.Word[2] &&
               lhs.host.u.Word[3] == rhs.host.u.Word[3] &&
               lhs.host.u.Word[4] == rhs.host.u.Word[4] &&
               lhs.host.u.Word[5] == rhs.host.u.Word[5] &&
               lhs.host.u.Word[6] == rhs.host.u.Word[6] &&
               lhs.host.u.Word[7] == rhs.host.u.Word[7] &&
               lhs.port == rhs.port;
    }
};

struct NetServerClient {
    ENetPeer *peer                          {};
    double    last_packet_received_at       {};
    bool      sent_welcome_basket           {};
    size_t    usernameLength                {};
    char      username[USERNAME_LENGTH_MAX] {};
};

struct NetServer {
    ENetHost *server{};
    std::unordered_map<ENetAddress, NetServerClient, NetAddressHash, NetAddressEqual> clients{};
    
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
    static const char *LOG_SRC;

    ErrorType SendRaw(const NetServerClient *client, const char *data, size_t size);
    ErrorType SendMsg(const NetServerClient *client, const NetMessage &message);
    ErrorType BroadcastRaw(const char *data, size_t size);
    ErrorType BroadcastMsg(const NetMessage &message);
    ErrorType BroadcastChatMessage(const char *msg, size_t msgLength);
    ErrorType SendWelcomeBasket(NetServerClient *client);
    void ProcessMsg(NetServerClient *client, Packet &packet);
};