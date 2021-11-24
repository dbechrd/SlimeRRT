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

    message.connectionToken = client.connectionToken;
    ENetBuffer rawPacket = message.Serialize(*serverWorld);
    //E_INFO("[SEND][%21s][%5u b] %16s ", TextFormatIP(client.peer->address), rawPacket.dataLength, netMsg.TypeString());
    E_ASSERT(SendRaw(client, rawPacket.data, rawPacket.dataLength), "Failed to send packet");
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

ErrorType NetServer::BroadcastChatMessage(const char *msg, size_t msgLength)
{
    assert(msg);
    assert(msgLength <= UINT32_MAX);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::ChatMessage;
    netMsg.data.chatMsg.usernameLength = sizeof(SV_USERNAME) - 1;
    memcpy(netMsg.data.chatMsg.username, CSTR(SV_USERNAME));
    netMsg.data.chatMsg.messageLength = (uint32_t)msgLength;
    memcpy(netMsg.data.chatMsg.message, msg, msgLength);
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendWelcomeBasket(NetServerClient &client)
{
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - slime list

    {
        memset(&netMsg, 0, sizeof(netMsg));
        netMsg.type = NetMessage::Type::Welcome;
        netMsg.data.welcome.motdLength = (uint32_t)(sizeof("Welcome to The Lonely Island") - 1);
        memcpy(netMsg.data.welcome.motd, CSTR("Welcome to The Lonely Island"));
        netMsg.data.welcome.width = serverWorld->map->width;
        netMsg.data.welcome.height = serverWorld->map->height;
        netMsg.data.welcome.playerId = client.playerId;
        E_ASSERT(SendMsg(client, netMsg), "Failed to send welcome basket");
    }

    // TODO: Save from identify packet into NetServerClient, then use client.username
    Player *player = serverWorld->FindPlayer(client.playerId);
    assert(player);
    if (player) {
        const char *message = TextFormat("%.*s joined the game.", player->nameLength, player->name);
        size_t messageLength = strlen(message);
        E_ASSERT(BroadcastChatMessage(message, messageLength), "Failed to broadcast join notification");
    }

    SendWorldChunk(client);
    //SendWorldSnapshot(client);

    return ErrorType::Success;
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
    netMsg.Deserialize(packetBuffer, *serverWorld);

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

            static uint32_t nextId = 1;
            Player *player = serverWorld->SpawnPlayer(nextId);
            if (player) {
                nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
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

            // TODO(security): Validate some session token that's not known to other people to prevent impersonation
            //assert(chatMsg.usernameLength == client.usernameLength);
            //assert(!strncmp(chatMsg.username, client.username, client.usernameLength));

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
                        printf("Received input #%u\n", histSample.seq);
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
            client.lastInputAck = serverWorld->tick - 1;
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