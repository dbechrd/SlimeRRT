#pragma once
#include "chat.h"
#include "error.h"
#include "item_world.h"
#include "dlb_murmur3.h"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

struct NetServerClient {
    ENetPeer *peer               {};
    uint32_t  connectionToken    {};  // unique identifier in addition to ip/port to detect reconnect from same UDP port
    uint32_t  playerId           {};
    uint32_t  lastInputAck       {};  // sequence # of last processed input for this client
    double    lastSnapshotSentAt {};

    //RingBuffer<WorldSnapshot, SV_WORLD_HISTORY> worldHistory {};
    std::unordered_map<uint32_t, PlayerSnapshot> playerHistory {};
    std::unordered_map<uint32_t, EnemySnapshot>  enemyHistory  {};
    std::unordered_map<uint32_t, ItemSnapshot>   itemHistory   {};
    std::unordered_set<ChunkHash>                chunkHistory  {};
};

struct NetServer {
    ENetHost        *server      {};
    World           *serverWorld {};
    uint32_t        nextPlayerId = 1;
    NetServerClient clients[SV_MAX_PLAYERS]{};
    RingBuffer<InputSample, SV_INPUT_HISTORY> inputHistory {};

    ~NetServer();
    ErrorType        OpenSocket        (unsigned short socketPort);
    ErrorType        SendWorldChunk    (const NetServerClient &client, const Chunk &chunk);
    void             SendNearbyChunks  (NetServerClient &client);
    ErrorType        SendWorldSnapshot (NetServerClient &client);
    //ErrorType        SendNearbyEvents  (const NetServerClient &client);
    NetServerClient *FindClient        (uint32_t playerId);
    ErrorType        Listen            (void);
    void             CloseSocket       (void);

private:
    static const char *LOG_SRC;
    NetMessage netMsg {};

    ErrorType SendRaw              (const NetServerClient &client, const void *data, size_t size);
    ErrorType SendMsg              (const NetServerClient &client, NetMessage &message);
    ErrorType BroadcastRaw         (const void *data, size_t size);
    ErrorType BroadcastMsg         (NetMessage &message);
    ErrorType SendWelcomeBasket    (NetServerClient &client);
    ErrorType BroadcastChatMessage (NetMessage_ChatMessage &chatMsg);
    ErrorType BroadcastPlayerJoin  (const Player &player);
    ErrorType BroadcastPlayerLeave (const Player &player);
    ErrorType SendPlayerState      (const NetServerClient &client, const Player &otherPlayer, bool nearby, bool spawned);
    ErrorType SendEnemyState       (const NetServerClient &client, const Slime &enemy, bool nearby, bool spawned);
    ErrorType SendItemState        (const NetServerClient &client, const ItemWorld &item, bool nearby, bool spawned);

    bool IsValidInput (const NetServerClient &client, const InputSample &sample);
    void ProcessMsg   (NetServerClient &client, ENetPacket &packet);

    NetServerClient *AddClient    (ENetPeer *peer);
    NetServerClient *FindClient   (ENetPeer *peer);
    ErrorType        RemoveClient (ENetPeer *peer);
};