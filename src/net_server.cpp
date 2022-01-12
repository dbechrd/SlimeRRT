#include "net_server.h"
#include "bit_stream.h"
#include "helpers.h"
#include "error.h"
#include "raylib/raylib.h"
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
    ENetAddress address{};
    //address.host = ENET_HOST_ANY;
    address.host = enet_v4_anyaddr;
    //address.host = enet_v4_localhost;
    address.port = socketPort;

    server = enet_host_create(&address, SV_MAX_PLAYERS, 1, 0, 0);
    while ((!server || !server->socket)) {
        E_ASSERT(ErrorType::HostCreateFailed, "Failed to create host. Check if port(s) %hu already in use.", socketPort);
    }
    printf("Listening on port %hu...\n", address.port);
    return ErrorType::Success;
}

ErrorType NetServer::SendRaw(const NetServerClient &client, const void *data, size_t size)
{
    assert(data);
    assert(size <= PACKET_SIZE_MAX);

    if (!client.peer || client.peer->state != ENET_PEER_STATE_CONNECTED) { // || !clients[i].playerId) {
        return ErrorType::Success;
    }

    assert(client.peer->address.port);

    // TODO(dlb): Don't always use reliable flag.. figure out what actually needs to be reliable (e.g. chat)
    ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
        E_ASSERT(ErrorType::PacketCreateFailed, "Failed to create packet.");
    }
    if (enet_peer_send(client.peer, 0, packet) < 0) {
        E_ASSERT(ErrorType::PeerSendFailed, "Failed to send connection request.");
    }
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastRaw(const void *data, size_t size)
{
    assert(data);
    assert(size <= PACKET_SIZE_MAX);

    ErrorType err_code = ErrorType::Success;

    // TODO(dlb): Don't always use reliable flag.. figure out what actually needs to be reliable (e.g. chat)
    ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
        E_ASSERT(ErrorType::PacketCreateFailed, "Failed to create packet.");
    }

    // Broadcast netMsg to all connected clients
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        if (!clients[i].peer || clients[i].peer->state != ENET_PEER_STATE_CONNECTED) { // || !clients[i].playerId) {
            continue;
        }

        NetServerClient &client = clients[i];
        assert(client.peer);
        assert(client.peer->address.port);
        if (enet_peer_send(client.peer, 0, packet) < 0) {
            TraceLog(LOG_ERROR, "[NetServer] BROADCAST %u bytes failed", size);
            err_code = ErrorType::PeerSendFailed;
        }
    }

    return err_code;
}

ErrorType NetServer::SendMsg(const NetServerClient &client, NetMessage &message)
{
    if (!client.peer || client.peer->state != ENET_PEER_STATE_CONNECTED) { // || !client.playerId) {
        return ErrorType::Success;
    }

    if (message.type != NetMessage::Type::WorldSnapshot) {
        const char *subType = "";
        switch (message.type) {
            case NetMessage::Type::GlobalEvent: subType = message.data.globalEvent.TypeString(); break;
            case NetMessage::Type::NearbyEvent: subType = message.data.nearbyEvent.TypeString(); break;
        }
        TraceLog(LOG_DEBUG, "[NetServer] Send %s %s", message.TypeString(), subType);
    }
    if (message.type == NetMessage::Type::Input) {
        assert((int)message.type);
    }

    message.connectionToken = client.connectionToken;
    ENetBuffer rawPacket{ PACKET_SIZE_MAX, calloc(PACKET_SIZE_MAX, sizeof(uint8_t)) };
    message.Serialize(*serverWorld, rawPacket);
    //E_INFO("[SEND][%21s][%5u b] %16s ", TextFormatIP(client.peer->address), rawPacket.dataLength, netMsg.TypeString());
    E_ASSERT(SendRaw(client, rawPacket.data, rawPacket.dataLength), "Failed to send packet");
    free(rawPacket.data);
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastMsg(NetMessage &message)
{
    ErrorType err_code = ErrorType::Success;

    // Broadcast netMsg to all connected clients
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        NetServerClient &client = clients[i];
        ErrorType result = SendMsg(clients[i], message);
        if (result != ErrorType::Success) {
            err_code = result;
        }
    }

    return err_code;
}

ErrorType NetServer::SendWelcomeBasket(NetServerClient &client)
{
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - slime list

    {
        memset(&netMsg, 0, sizeof(netMsg));
        netMsg.type = NetMessage::Type::Welcome;

        NetMessage_Welcome &welcome = netMsg.data.welcome;
        welcome.motdLength = (uint32_t)(sizeof("Welcome to The Lonely Island") - 1);
        memcpy(welcome.motd, CSTR("Welcome to The Lonely Island"));
        welcome.width = serverWorld->map->width;
        welcome.height = serverWorld->map->height;
        welcome.playerId = client.playerId;
        welcome.playerCount = 0;
        for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
            if (!serverWorld->players[i].id)
                continue;

            welcome.players[i].id = serverWorld->players[i].id;
            welcome.players[i].nameLength = serverWorld->players[i].nameLength;
            memcpy(welcome.players[i].name, serverWorld->players[i].name, welcome.players[i].nameLength);
            welcome.playerCount++;
        }
        E_ASSERT(SendMsg(client, netMsg), "Failed to send welcome basket");
    }

    Player *player = serverWorld->FindPlayer(client.playerId);
    if (!player) {
        TraceLog(LOG_FATAL, "Failed to find player. playerId: %u", client.playerId);
    }

    E_ASSERT(BroadcastPlayerJoin(*player), "Failed to broadcast player join notification");

    const char *message = TextFormat("%.*s joined the game.", player->nameLength, player->name);
    size_t messageLength = strlen(message);
    NetMessage_ChatMessage chatMsg{};
    chatMsg.source = NetMessage_ChatMessage::Source::Server;
    chatMsg.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
    memcpy(chatMsg.message, message, chatMsg.messageLength);
    E_ASSERT(BroadcastChatMessage(chatMsg), "Failed to broadcast player join chat msg");

    // Initial "snapshot" of the world
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        if (serverWorld->players[i].id &&
            v3_length_sq(v3_sub(player->body.position, serverWorld->players[i].body.position)) <= SQUARED(SV_PLAYER_NEARBY_THRESHOLD))
        {
            E_ASSERT(SendPlayerSpawn(client, serverWorld->players[i].id), "Failed to send player spawn notification");
        }
    }

    for (size_t i = 0; i < SV_MAX_SLIMES; i++) {
        if (serverWorld->slimes[i].id &&
            v3_length_sq(v3_sub(player->body.position, serverWorld->slimes[i].body.position)) <= SQUARED(SV_ENEMY_NEARBY_THRESHOLD)) {
            E_ASSERT(SendEnemySpawn(client, serverWorld->slimes[i].id), "Failed to send enemy spawn notification");
        }
    }

    SendWorldChunk(client);
    //SendWorldSnapshot(client);

    return ErrorType::Success;
}

ErrorType NetServer::BroadcastChatMessage(NetMessage_ChatMessage &chatMsg)
{
    assert(chatMsg.message);
    assert(chatMsg.messageLength <= UINT32_MAX);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::ChatMessage;
    netMsg.data.chatMsg = chatMsg;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::BroadcastPlayerJoin(const Player &player)
{
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::GlobalEvent;

    NetMessage_GlobalEvent &globalEvent = netMsg.data.globalEvent;
    globalEvent.type = NetMessage_GlobalEvent::Type::PlayerJoin;

    NetMessage_GlobalEvent_PlayerJoin &playerJoin = netMsg.data.globalEvent.data.playerJoin;
    playerJoin.playerId = player.id;
    playerJoin.nameLength = player.nameLength;
    memcpy(playerJoin.name, player.name, player.nameLength);
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::BroadcastPlayerLeave(uint32_t playerId)
{
    assert(playerId);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::GlobalEvent;

    NetMessage_GlobalEvent &globalEvent = netMsg.data.globalEvent;
    globalEvent.type = NetMessage_GlobalEvent::Type::PlayerLeave;

    NetMessage_GlobalEvent_PlayerLeave &playerLeave = netMsg.data.globalEvent.data.playerLeave;
    playerLeave.playerId = playerId;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendPlayerSpawn(NetServerClient &client, uint32_t playerId)
{
    assert(playerId);

    Player *player = serverWorld->FindPlayer(playerId);
    if (!player) {
        TraceLog(LOG_ERROR, "Cannot find player we're trying to generate spawn notification about");
        return ErrorType::PlayerNotFound;
    }

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::NearbyEvent;

    NetMessage_NearbyEvent &nearbyEvent = netMsg.data.nearbyEvent;
    nearbyEvent.type = NetMessage_NearbyEvent::Type::PlayerSpawn;

    NetMessage_NearbyEvent_PlayerSpawn &playerSpawn = netMsg.data.nearbyEvent.data.playerSpawn;
    playerSpawn.playerId = player->id;
    playerSpawn.position = player->body.position;
    playerSpawn.direction = player->sprite.direction;
    playerSpawn.hitPoints = player->combat.hitPoints;
    playerSpawn.hitPointsMax = player->combat.hitPointsMax;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendPlayerDespawn(NetServerClient &client, uint32_t playerId)
{
    assert(playerId);

    Player *player = serverWorld->FindPlayer(playerId);
    if (!player) {
        TraceLog(LOG_ERROR, "Cannot find player we're trying to generate despawn notification about");
        return ErrorType::PlayerNotFound;
    }

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::NearbyEvent;

    NetMessage_NearbyEvent &nearbyEvent = netMsg.data.nearbyEvent;
    nearbyEvent.type = NetMessage_NearbyEvent::Type::PlayerDespawn;

    NetMessage_NearbyEvent_PlayerDespawn &playerDespawn = netMsg.data.nearbyEvent.data.playerDespawn;
    playerDespawn.playerId = player->id;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendEnemySpawn(NetServerClient &client, uint32_t enemyId)
{
    assert(enemyId);

    Slime *slime = serverWorld->FindSlime(enemyId);
    if (!slime) {
        TraceLog(LOG_ERROR, "Cannot find enemy we're trying to generate spawn notification about");
        return ErrorType::EnemyNotFound;
    }

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::NearbyEvent;

    NetMessage_NearbyEvent &nearbyEvent = netMsg.data.nearbyEvent;
    nearbyEvent.type = NetMessage_NearbyEvent::Type::EnemySpawn;

    NetMessage_NearbyEvent_EnemySpawn &enemySpawn = netMsg.data.nearbyEvent.data.enemySpawn;
    enemySpawn.enemyId = slime->id;
    enemySpawn.position = slime->body.position;
    enemySpawn.direction = slime->sprite.direction;
    enemySpawn.hitPoints = slime->combat.hitPoints;
    enemySpawn.hitPointsMax = slime->combat.hitPointsMax;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendWorldChunk(NetServerClient &client)
{
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::WorldChunk;
    netMsg.data.worldChunk.offsetX = 0;
    netMsg.data.worldChunk.offsetY = 0;

    uint32_t tileCount = (uint32_t)MIN(serverWorld->map->width * serverWorld->map->height, WORLD_CHUNK_TILES_MAX);
    netMsg.data.worldChunk.tilesLength = tileCount;
    E_ASSERT(SendMsg(client, netMsg), "Failed to send world chunks");
    return ErrorType::Success;
}

ErrorType NetServer::SendWorldSnapshot(NetServerClient &client, WorldSnapshot &worldSnapshot)
{
    assert(client.playerId);
    assert(worldSnapshot.playerId == client.playerId);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::WorldSnapshot;
    // TODO: Get rid of this memcpy?
    memcpy(&netMsg.data.worldSnapshot, &worldSnapshot, sizeof(netMsg.data.worldSnapshot));
    E_ASSERT(SendMsg(client, netMsg), "Failed to send world snapshot");

    client.lastSnapshotSentAt = glfwGetTime();
    return ErrorType::Success;
}

NetServerClient *NetServer::FindClient(uint32_t playerId)
{
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        if (clients[i].playerId == playerId) {
            return &clients[i];
        }
    }
    return 0;
}

bool NetServer::IsValidInput(const NetServerClient &client, const InputSample &sample)
{
    // If ownerId doesn't match, ignore
    if (sample.ownerId != client.playerId) {
        // TODO: Disconnect someone trying to impersonate?
        // Maybe their ID changed due to a recent reconnect? Check connection age.
        return false;
    }

    // If sample with this tick has already been ack'd (or is past due), ignore
    if (sample.seq <= client.lastInputAck) {
        // TODO: Disconnect someone trying to send exceptionally old inputs?
        return false;
    }

    // If sample is too far in the future, ignore
    //if (sample.seq > client.lastInputAck + (1000 / SV_TICK_RATE)) {
    //    // TODO: Disconnect someone trying to send futuristic inputs?
    //    printf("Input too far in future.. ignoring (%u > %u)\n", sample.clientTick, client.lastInputAck);
    //    return false;
    //}

    // If sample already exists, ignore
    for (size_t i = 0; i < inputHistory.Count(); i++) {
        InputSample &histSample = inputHistory.At(i);
        if (histSample.ownerId == sample.ownerId && histSample.seq == sample.seq) {
            return false;
        }
    }

    return true;
}

void NetServer::ProcessMsg(NetServerClient &client, ENetPacket &packet)
{
    assert(serverWorld);

    ENetBuffer packetBuffer{ packet.dataLength, packet.data };
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.Deserialize(*serverWorld, packetBuffer);

    if (netMsg.type != NetMessage::Type::Identify &&
        netMsg.connectionToken != client.connectionToken)
    {
        // Received a netMsg from a stale connection; discard it
        printf("Ignoring %s packet from stale connection.\n", netMsg.TypeString());
        return;
    }

    //E_INFO("[RECV][%21s][%5u b] %16s ", TextFormatIP(client.peer->address), packet.rawBytes.dataLength, netMsg.TypeString());

    switch (netMsg.type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = netMsg.data.identify;

            Player *player = serverWorld->AddPlayer(nextPlayerId);
            if (player) {
                player->body.position = serverWorld->GetWorldSpawn();
                nextPlayerId = MAX(1, nextPlayerId + 1); // Prevent ID zero from being used on overflow
                client.playerId = player->id;
                client.connectionToken = netMsg.connectionToken;
                assert(identMsg.usernameLength);
                player->SetName(identMsg.username, identMsg.usernameLength);
                SendWelcomeBasket(client);
            } else {
                // TODO: Send server full netMsg
            }

            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = netMsg.data.chatMsg;

            // TODO(security): Detect someone sending packets with wrong source/id and PUNISH THEM (.. or wutevs)
            chatMsg.source = NetMessage_ChatMessage::Source::Client;
            chatMsg.id = client.playerId;

            // Store chat netMsg in chat history
            serverWorld->chatHistory.PushNetMessage(chatMsg);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat netMsg
            BroadcastMsg(netMsg);
            break;
        } case NetMessage::Type::Input: {
            NetMessage_Input &input = netMsg.data.input;
            if (input.sampleCount <= CL_INPUT_SAMPLES_MAX) {
                for (size_t i = 0; i < input.sampleCount; i++) {
                    InputSample &sample = input.samples[i];
                    if (sample.seq && IsValidInput(client, sample)) {
                        InputSample &histSample = inputHistory.Alloc();
                        histSample = sample;
#if SV_DEBUG_INPUT
                        printf("Received input #%u\n", histSample.seq);
#endif
                    }
                }
            } else {
                // Invalid message format, disconnect naughty user?
            }
            break;
        } default: {
            E_INFO("Unexpected netMsg type: %s\n", netMsg.TypeString());
            break;
        }
    }
}

NetServerClient *NetServer::AddClient(ENetPeer *peer)
{
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        NetServerClient &client = clients[i];
        if (!client.playerId) {
            assert(!client.peer);
            client.peer = peer;
            peer->data = &client;

            assert(serverWorld->tick);
            return &client;
        }
    }
    return 0;
}

NetServerClient *NetServer::FindClient(ENetPeer *peer)
{
    NetServerClient *client = (NetServerClient *)peer->data;
    if (client) {
        return client;
    }

    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        if (clients[i].peer == peer) {
            return &clients[i];
        }
    }
    return 0;
}

ErrorType NetServer::RemoveClient(ENetPeer *peer)
{
    printf("Remove client %s\n", TextFormatIP(peer->address));
    NetServerClient *client = FindClient(peer);
    if (client) {
        // TODO: Save from identify packet into NetServerClient, then user client.username
        Player *player = serverWorld->FindPlayer(client->playerId);
        if (player) {
            E_ASSERT(BroadcastPlayerLeave(player->id), "Failed to broadcast player leave notification");

            const char *message = TextFormat("%.*s left the game.", player->nameLength, player->name);
            size_t messageLength = strlen(message);
            NetMessage_ChatMessage chatMsg{};
            chatMsg.source = NetMessage_ChatMessage::Source::Server;
            chatMsg.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
            memcpy(chatMsg.message, message, chatMsg.messageLength);
            E_ASSERT(BroadcastChatMessage(chatMsg), "Failed to broadcast player leave chat msg");
        }

        serverWorld->RemovePlayer(client->playerId);
        memset(client, 0, sizeof(*client));
    }
    peer->data = 0;
    enet_peer_reset(peer);
    return ErrorType::Success;
}

ErrorType NetServer::Listen()
{
    assert(server->address.port);

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?

    ENetEvent event{};
    // TODO(dlb): How long should this wait between calls?
    int svc = enet_host_service(server, &event, 1);
    while (svc > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                E_INFO("A new client connected from %x:%u.", TextFormatIP(event.peer->address));
                AddClient(event.peer);
                break;
            } case ENET_EVENT_TYPE_RECEIVE: {
                //E_INFO("A packet of length %u was received from %x:%u on channel %u.",
                //    event.packet->dataLength,
                //    event.peer->address.host,
                //    event.peer->address.port,
                //    event.channelID);

                NetServerClient *client = FindClient(event.peer);
                assert(client);
                if (client) {
                    ProcessMsg(*client, *event.packet);
                }
                enet_packet_destroy(event.packet);
                break;
            } case ENET_EVENT_TYPE_DISCONNECT: {
                E_INFO("%x:%u disconnected.", TextFormatIP(event.peer->address));
                RemoveClient(event.peer);
                break;
            } case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                E_INFO("%x:%u disconnected due to timeout.", TextFormatIP(event.peer->address));
                RemoveClient(event.peer);
                break;
            } default: {
                E_WARN("Unhandled event type: %d", event.type);
                break;
            }
        }
        svc = enet_host_service(server, &event, 0);
    }

    return ErrorType::Success;
}

void NetServer::CloseSocket()
{
    // Notify all clients that the server is stopping
    for (int i = 0; i < server->peerCount; i++) {
        enet_peer_disconnect(&server->peers[i], 0);
    }
    enet_host_service(server, nullptr, 0);
    enet_host_destroy(server);
    assert(sizeof(clients) > 8); // in case i change client list to a pointer and break the memset
    memset(clients, 0, sizeof(clients));
}