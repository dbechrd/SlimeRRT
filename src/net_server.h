#pragma once
#include "chat.h"
#include "error.h"
#include "item_world.h"
#include "dlb_murmur3.h"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

struct SV_Client {
    ENetPeer    *peer              {};
    uint32_t    connectionToken    {};  // unique identifier in addition to ip/port to detect reconnect from same UDP port
    uint32_t    playerId           {};
    uint32_t    lastInputAck       {};  // sequence # of last input processed for this client
    uint32_t    lastInputRecv      {};  // sequence # of last input received from client
    double      lastSnapshotSentAt {};
    float       inputOverflow      {};  // how msec of input we've received over/under expected by frameDt

    //InputSample inputBuffer        {};  // last input received (TODO: all input received since last tick, consolidated)
    RingBuffer<InputSample, SV_INPUT_HISTORY> inputHistory {};

    //RingBuffer<WorldSnapshot, SV_WORLD_HISTORY> worldHistory {};
    std::unordered_map<uint32_t, PlayerSnapshot> playerHistory {};
    std::unordered_map<uint32_t, EnemySnapshot>  enemyHistory  {};
    std::unordered_map<EntityUID, ItemSnapshot>  itemHistory   {};
    std::unordered_set<ChunkHash>                chunkHistory  {};  // TODO: RingBuffer, this set will grow indefinitely
};

struct NetServer {
    ENetHost  *server      {};
    World     *serverWorld {};
    SV_Client clients[SV_MAX_PLAYERS]{};
    //RingBuffer<InputSample, SV_INPUT_HISTORY> inputHistory {};

    NetServer                   (void);
    ~NetServer                  (void);
    ErrorType OpenSocket        (unsigned short socketPort);
    ErrorType SendChatMessage   (const SV_Client &client, const char *message, size_t messageLength);
    ErrorType SendWorldChunk    (const SV_Client &client, const Chunk &chunk);
    void      SendNearbyChunks  (SV_Client &client);
    ErrorType SendWorldSnapshot (SV_Client &client);
    //ErrorType SendNearbyEvents  (const SV_Client &client);
    SV_Client *FindClient       (uint32_t playerId);
    ErrorType Listen            (void);
    void      CloseSocket       (void);

private:
    static const char *LOG_SRC;
    NetMessage netMsg {};
    ENetBuffer rawPacket {};
    struct {
        uint32_t length;
        uint8_t *data;
    } fbs_users;

    ErrorType SaveUserData(const char *filename);
    ErrorType LoadUserData(const char *filename);

    ErrorType SendRaw              (const SV_Client &client, const void *data, size_t size);
    ErrorType SendMsg              (const SV_Client &client, NetMessage &message);
    ErrorType BroadcastRaw         (const void *data, size_t size);
    ErrorType BroadcastMsg         (NetMessage &message);
    ErrorType SendWelcomeBasket    (SV_Client &client);
    ErrorType BroadcastChatMessage (NetMessage_ChatMessage &chatMsg);
    ErrorType BroadcastPlayerJoin  (const PlayerInfo &playerInfo);
    ErrorType BroadcastPlayerLeave (uint32_t playerId);
    ErrorType SendPlayerState      (const SV_Client &client, const Player &otherPlayer, bool nearby, bool spawned);
    ErrorType SendEnemyState       (const SV_Client &client, const Enemy &enemy, bool nearby, bool spawned);
    ErrorType SendItemState        (const SV_Client &client, const ItemWorld &item, bool nearby, bool spawned);

    bool IsValidInput (const SV_Client &client, const InputSample &sample);
    bool ParseCommand (SV_Client &client, NetMessage_ChatMessage &chatMsg);
    void ProcessMsg   (SV_Client &client, ENetPacket &packet);

    SV_Client *AddClient   (ENetPeer *peer);
    SV_Client *FindClient  (ENetPeer *peer);
    ErrorType RemoveClient (ENetPeer *peer);
};