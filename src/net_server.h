#pragma once
#include "chat.h"
#include "error.h"
#include "dlb_murmur3.h"
#include <cstdint>
#include <unordered_map>

//struct NetAddressHash {
//    std::size_t operator()(const ENetAddress &address) const
//    {
//        return (size_t)dlb_murmur3(&address.host, sizeof(address.host)) ^
//              ((size_t)dlb_murmur3(&address.port, sizeof(address.port)) << 1);
//    }
//};
//
//struct NetAddressEqual {
//    bool operator()(const ENetAddress &lhs, const ENetAddress &rhs) const
//    {
//        return lhs.host.u.Word[0] == rhs.host.u.Word[0] &&
//               lhs.host.u.Word[1] == rhs.host.u.Word[1] &&
//               lhs.host.u.Word[2] == rhs.host.u.Word[2] &&
//               lhs.host.u.Word[3] == rhs.host.u.Word[3] &&
//               lhs.host.u.Word[4] == rhs.host.u.Word[4] &&
//               lhs.host.u.Word[5] == rhs.host.u.Word[5] &&
//               lhs.host.u.Word[6] == rhs.host.u.Word[6] &&
//               lhs.host.u.Word[7] == rhs.host.u.Word[7] &&
//               lhs.port == rhs.port;
//    }
//};

struct NetServerClient {
    ENetPeer *peer               {};
    uint32_t  connectionToken    {};  // unique identifier in addition to ip/port to detect reconnect from same UDP port
    uint32_t  playerId           {};
    uint32_t  lastInputAck       {};  // sequence # of last processed input for this client
    double    lastSnapshotSentAt {};
    RingBuffer<WorldSnapshot, SV_WORLD_HISTORY> worldHistory {};
};

struct NetServer {
    ENetHost *server {};
    //std::unordered_map<ENetAddress, NetServerClient, NetAddressHash, NetAddressEqual> clients{};
    uint32_t nextPlayerId = 1;
    NetServerClient clients[SV_MAX_PLAYERS]{};
    RingBuffer<InputSample, SV_INPUT_HISTORY> inputHistory {};
    World *serverWorld {};

    ~NetServer();
    ErrorType OpenSocket(unsigned short socketPort);
    ErrorType SendWorldChunk(NetServerClient &client);
    ErrorType SendWorldSnapshot(NetServerClient &client, WorldSnapshot &worldSnapshot);
    NetServerClient *FindClient(uint32_t playerId);
    ErrorType Listen();
    void CloseSocket();

private:
    static const char *LOG_SRC;
    NetMessage netMsg {};

    ErrorType SendRaw(const NetServerClient &client, const void *data, size_t size);
    ErrorType SendMsg(const NetServerClient &client, NetMessage &message);
    ErrorType BroadcastRaw(const void *data, size_t size);
    ErrorType BroadcastMsg(NetMessage &message);
    ErrorType SendWelcomeBasket(NetServerClient &client);
    ErrorType BroadcastChatMessage(const char *msg, size_t msgLength);
    ErrorType BroadcastPlayerJoin(const Player &player);
    ErrorType BroadcastPlayerLeave(uint32_t playerId);
    ErrorType SendPlayerSpawn(NetServerClient &client, uint32_t playerId);

    bool IsValidInput(const NetServerClient &client, const InputSample &sample);
    void ProcessMsg(NetServerClient &client, ENetPacket &packet);

    NetServerClient *AddClient    (ENetPeer *peer);
    NetServerClient *FindClient   (ENetPeer *peer);
    ErrorType        RemoveClient (ENetPeer *peer);
};