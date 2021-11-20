#include "net_server.h"
#include "bit_stream.h"
#include "helpers.h"
#include "packet.h"
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

    server = enet_host_create(&address, SERVER_MAX_PLAYERS, 1, 0, 0);
    if (!server || !server->socket) {
        E_ASSERT(ErrorType::HostCreateFailed, "Failed to create host.");
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

    // Broadcast message to all connected clients
    for (int i = 0; i < SERVER_MAX_PLAYERS; i++) {
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
    message.connectionToken = client.connectionToken;
    ENetBuffer rawPacket = message.Serialize(*serverWorld);
    //E_INFO("[SEND][%21s][%5u b] %16s ", TextFormatIP(client.peer->address), rawPacket.dataLength, message.TypeString());
    E_ASSERT(SendRaw(client, rawPacket.data, rawPacket.dataLength), "Failed to send packet");
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastMsg(NetMessage &message)
{
    ErrorType err_code = ErrorType::Success;

    // Broadcast message to all connected clients
    for (int i = 0; i < SERVER_MAX_PLAYERS; i++) {
        NetServerClient &client = clients[i];
        ErrorType result = SendMsg(clients[i], message);
        if (result != ErrorType::Success) {
            err_code = result;
        }
    }

    return err_code;
}

ErrorType NetServer::BroadcastChatMessage(const char *msg, size_t msgLength)
{
    assert(msg);
    assert(msgLength <= UINT32_MAX);

    NetMessage netMessage{};
    netMessage.type = NetMessage::Type::ChatMessage;
    netMessage.data.chatMsg.usernameLength = sizeof(SERVER_USERNAME) - 1;
    memcpy(netMessage.data.chatMsg.username, CSTR(SERVER_USERNAME));
    netMessage.data.chatMsg.messageLength = (uint32_t)msgLength;
    memcpy(netMessage.data.chatMsg.message, msg, msgLength);
    return BroadcastMsg(netMessage);
}

ErrorType NetServer::SendWelcomeBasket(NetServerClient &client)
{
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - player list

    {
        NetMessage userWelcomeBasket{};
        userWelcomeBasket.type = NetMessage::Type::Welcome;
        userWelcomeBasket.data.welcome.motdLength = (uint32_t)(sizeof("Welcome to The Lonely Island") - 1);
        memcpy(userWelcomeBasket.data.welcome.motd, CSTR("Welcome to The Lonely Island"));
        userWelcomeBasket.data.welcome.width = serverWorld->map->width;
        userWelcomeBasket.data.welcome.height = serverWorld->map->height;
        userWelcomeBasket.data.welcome.playerId = client.playerId;
        E_ASSERT(SendMsg(client, userWelcomeBasket), "Failed to send welcome basket");
    }

    {
        // TODO: Save from identify packet into NetServerClient, then user client.username
        Player *player = serverWorld->FindPlayer(client.playerId);
        assert(player);
        if (player) {
            const char *message = TextFormat("%.*s joined the game.", player->nameLength, player->name);
            size_t messageLength = strlen(message);
            E_ASSERT(BroadcastChatMessage(message, messageLength), "Failed to broadcast join notification");
        }
    }

    BroadcastWorldChunk();
    BroadcastWorldPlayers();
    BroadcastWorldEntities();

    return ErrorType::Success;
}

ErrorType NetServer::BroadcastWorldChunk(void)
{
    NetMessage worldChunk{};
    worldChunk.type = NetMessage::Type::WorldChunk;
    worldChunk.data.worldChunk.offsetX = 0;
    worldChunk.data.worldChunk.offsetY = 0;

    uint32_t tileCount = (uint32_t)MIN(serverWorld->map->width * serverWorld->map->height, WORLD_CHUNK_TILES_MAX);
    worldChunk.data.worldChunk.tilesLength = tileCount;
    E_ASSERT(BroadcastMsg(worldChunk), "Failed to send world chunks");
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastWorldPlayers(void)
{
    if (worldHistory.Count()) {
        WorldSnapshot &worldSnapshot = worldHistory.Last();
        NetMessage worldPlayers{};
        worldPlayers.type = NetMessage::Type::WorldPlayers;
        worldPlayers.data.worldPlayers.tick = worldSnapshot.tick;
        memcpy(worldPlayers.data.worldPlayers.players, worldSnapshot.players, sizeof(worldPlayers.data.worldPlayers.players));
        E_ASSERT(BroadcastMsg(worldPlayers), "Failed to send world players");
    }
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastWorldEntities(void)
{
    if (worldHistory.Count()) {
        WorldSnapshot &worldSnapshot = worldHistory.Last();
        NetMessage worldEntities{};
        worldEntities.type = NetMessage::Type::WorldEntities;
        worldEntities.data.worldEntities.tick = worldSnapshot.tick;
        memcpy(worldEntities.data.worldEntities.slimes, worldSnapshot.slimes, sizeof(worldEntities.data.worldEntities.slimes));
        E_ASSERT(BroadcastMsg(worldEntities), "Failed to send world entities");
    }
    return ErrorType::Success;
}

void NetServer::ProcessMsg(NetServerClient &client, ENetPacket &packet)
{
    ENetBuffer packetBuffer{ packet.dataLength, packet.data };
    NetMessage message{};
    message.Deserialize(packetBuffer, *serverWorld);

    if (message.type != NetMessage::Type::Identify &&
        message.connectionToken != client.connectionToken)
    {
        // Received a message from a stale connection; discard it
        printf("Ignoring %s packet from stale connection.\n", message.TypeString());
        return;
    }

    //E_INFO("[RECV][%21s][%5u b] %16s ", TextFormatIP(client.peer->address), packet.rawBytes.dataLength, message.TypeString());

    switch (message.type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = message.data.identify;

            Player *player = serverWorld->SpawnPlayer();
            if (player) {
                client.playerId = player->id;
                client.connectionToken = message.connectionToken;
                assert(identMsg.usernameLength);
                player->SetName(identMsg.username, identMsg.usernameLength);
                SendWelcomeBasket(client);
            } else {
                // TODO: Send server full message
            }

            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = message.data.chatMsg;

            // TODO(security): Validate some session token that's not known to other people to prevent impersonation
            //assert(chatMsg.usernameLength == client.usernameLength);
            //assert(!strncmp(chatMsg.username, client.username, client.usernameLength));

            // Store chat message in chat history
            chatHistory.PushNetMessage(chatMsg);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat message
            BroadcastMsg(message);
            break;
        } case NetMessage::Type::Input: {
            // TODO: Save all unprocessed input (in a queue, with timestamp), not just latest input
            NetMessage_Input &input = client.inputHistory.Alloc();
            input.walkNorth  = message.data.input.walkNorth;
            input.walkEast   = message.data.input.walkEast;
            input.walkSouth  = message.data.input.walkSouth;
            input.walkWest   = message.data.input.walkWest;
            input.run        = message.data.input.run;
            input.attack     = message.data.input.attack;
            input.selectSlot = message.data.input.selectSlot;
            break;
        } default: {
            E_INFO("Unexpected message type: %s\n", message.TypeString());
            break;
        }
    }
}

NetServerClient *NetServer::AddClient(ENetPeer *peer)
{
    for (int i = 0; i < SERVER_MAX_PLAYERS; i++) {
        if (!clients[i].playerId) {
            assert(!clients[i].peer);
            clients[i].peer = peer;
            peer->data = &clients[i];
            return &clients[i];
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

    for (int i = 0; i < SERVER_MAX_PLAYERS; i++) {
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
            const char *message = TextFormat("%.*s left the game.", player->nameLength, player->name);
            size_t messageLength = strlen(message);
            E_ASSERT(BroadcastChatMessage(message, messageLength), "Failed to broadcast join notification");
        }

        serverWorld->DespawnPlayer(client->playerId);
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

                //Packet &packet = packetHistory.Alloc();
                //packet.srcAddress = event.peer->address;
                //packet.timestamp = enet_time_get();
                //packet.rawBytes.data = calloc(event.packet->dataLength, sizeof(uint8_t));
                //memcpy(packet.rawBytes.data, event.packet->data, event.packet->dataLength);
                //packet.rawBytes.dataLength = event.packet->dataLength;

                // TODO: Could Packet just point to ENetPacket instead of copying and destroying?
                // When would ENetPacket get destroyed? Would that confuse ENet in some way?

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