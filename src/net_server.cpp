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
    if (!server) {
        E_ASSERT(ErrorType::HostCreateFailed, "Failed to create host.");
    }
    // TODO(dlb)[cleanup]: This probably isn't providing any additional value on top of if (!server) check
    assert(server->socket);
    return ErrorType::Success;
}

ErrorType NetServer::SendRaw(const NetServerClient &client, const void *data, size_t size)
{
    assert(client.peer);
    assert(client.peer->address.port);
    assert(data);
    assert(size <= PACKET_SIZE_MAX);

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

ErrorType NetServer::SendMsg(const NetServerClient &client, NetMessage &message)
{
    ENetBuffer rawPacket = message.Serialize();
    //E_INFO("[SEND][%21s][%5u b] %16s ", TextFormatIP(client.peer->address), rawPacket.dataLength, message.TypeString());
    E_ASSERT(SendRaw(client, rawPacket.data, rawPacket.dataLength), "Failed to send packet");
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastRaw(const void *data, size_t size)
{
    ErrorType err_code = ErrorType::Success;

    // Broadcast message to all connected clients
    for (auto &kv : clients) {
        assert(kv.second.peer);
        assert(kv.second.peer->address.port);
        ErrorType code = SendRaw(kv.second, data, size);
        if (code != ErrorType::Success) {
            TraceLog(LOG_ERROR, "[NetServer] BROADCAST %u bytes failed", size);
            err_code = code;
            // TODO: Handle error somehow? Retry queue.. or..? Idk.. This seems fatal. Look up why Win32 might fail
        }
    }

    return err_code;
}

ErrorType NetServer::BroadcastMsg(NetMessage &message)
{
    ENetBuffer rawPacket = message.Serialize();
    //E_INFO("[SEND][%21s][%5u b] %16s", "BROADCAST", rawPacket.dataLength, message.TypeString());
    return BroadcastRaw(rawPacket.data, rawPacket.dataLength);
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
        userWelcomeBasket.data.welcome.playerIdx = (uint32_t)client.playerIdx;
        E_ASSERT(SendMsg(client, userWelcomeBasket), "Failed to send welcome basket");
    }

    {
        // TODO: Save from identify packet into NetServerClient, then user client.username
        const char *clientUsername = client.username;
        size_t clientUsernameLength = client.usernameLength;
        const char *message = TextFormat("%.*s joined the game.", clientUsernameLength, clientUsername);
        size_t messageLength = strlen(message);
        E_ASSERT(BroadcastChatMessage(message, messageLength), "Failed to broadcast join notification");
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
    for (uint32_t i = 0; i < tileCount; i++) {
        worldChunk.data.worldChunk.tiles[i] = serverWorld->map->tiles[i];
    }
    E_ASSERT(BroadcastMsg(worldChunk), "Failed to send world chunks");
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastWorldPlayers(void)
{
    NetMessage worldPlayers{};
    worldPlayers.type = NetMessage::Type::WorldPlayers;

    worldPlayers.data.worldPlayers.playersLength = (uint32_t)serverWorld->playerCount;
    for (size_t i = 0; i < serverWorld->playerCount; i++) {
        worldPlayers.data.worldPlayers.players[i] = serverWorld->players[i];
    }
    E_ASSERT(BroadcastMsg(worldPlayers), "Failed to send world players");
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastWorldEntities(void)
{
    NetMessage worldEntities{};
    worldEntities.type = NetMessage::Type::WorldEntities;

    worldEntities.data.worldEntities.entitiesLength = (uint32_t)serverWorld->slimeCount;
    for (size_t i = 0; i < serverWorld->slimeCount; i++) {
        worldEntities.data.worldEntities.entities[i] = serverWorld->slimes[i];
    }
    E_ASSERT(BroadcastMsg(worldEntities), "Failed to send world entities");
    return ErrorType::Success;
}

void NetServer::ProcessMsg(NetServerClient &client, Packet &packet)
{
    packet.netMessage.Deserialize(packet.rawBytes);

    //E_INFO("[RECV][%21s][%5u b] %16s ", TextFormatIP(client.peer->address), packet.rawBytes.dataLength, packet.netMessage.TypeString());

    switch (packet.netMessage.type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = packet.netMessage.data.identify;
            client.usernameLength = MIN(identMsg.usernameLength, USERNAME_LENGTH_MAX);
            memcpy(client.username, identMsg.username, client.usernameLength);

            assert(serverWorld->SpawnPlayer(client.username, client.usernameLength, client.playerIdx));
            SendWelcomeBasket(client);

            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = packet.netMessage.data.chatMsg;

            // TODO(security): Validate some session token that's not known to other people to prevent impersonation
            //assert(chatMsg.usernameLength == client.usernameLength);
            //assert(!strncmp(chatMsg.username, client.username, client.usernameLength));

            // Store chat message in chat history
            chatHistory.PushNetMessage(chatMsg);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat message
            BroadcastMsg(packet.netMessage);
            break;
        } case NetMessage::Type::Input: {
            // TODO: Save all unprocessed input (in a queue, with timestamp), not just latest input
            NetMessage_Input &input = packet.netMessage.data.input;
            client.input.walkNorth  = input.walkNorth;
            client.input.walkEast   = input.walkEast;
            client.input.walkSouth  = input.walkSouth;
            client.input.walkWest   = input.walkWest;
            client.input.run        = input.run;
            client.input.attack     = input.attack;
            client.input.selectSlot = input.selectSlot;

            break;
        } default: {
            assert("asdfasdf");
            break;
        }
    }
}

NetServerClient &NetServer::FindClient(ENetPeer *peer)
{
    NetServerClient *client = (NetServerClient * )peer->data;
    if (!client) {
        auto kv = clients.find(peer->address);
        if (kv == clients.end()) {
            client = &clients[peer->address];
        } else {
            client = &kv->second;
        }
    }
    return *client;
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
                //E_INFO("A new client connected from %x:%u.",
                //    event.peer->address.host,
                //    event.peer->address.port);
                // TODO: Store any relevant client information here.

                NetServerClient &client = FindClient(event.peer);
                client.peer = event.peer;
                client.last_packet_received_at = enet_time_get();
                event.peer->data = &client;;
                break;
            } case ENET_EVENT_TYPE_RECEIVE: {
                //E_INFO("A packet of length %u was received from %x:%u on channel %u.",
                //    event.packet->dataLength,
                //    event.peer->address.host,
                //    event.peer->address.port,
                //    event.channelID);

                Packet &packet = packetHistory.Alloc();
                packet.srcAddress = event.peer->address;
                packet.timestamp = enet_time_get();
                packet.rawBytes.data = calloc(event.packet->dataLength, sizeof(uint8_t));
                memcpy(packet.rawBytes.data, event.packet->data, event.packet->dataLength);
                packet.rawBytes.dataLength = event.packet->dataLength;

                // TODO: Refactor this out into helper function somewhere (it's also in net_client.c)
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                int len = snprintf(packet.timestampStr, sizeof(packet.timestampStr), "%02d:%02d:%02d", tm.tm_hour,
                    tm.tm_min, tm.tm_sec);
                assert(len < sizeof(packet.timestampStr));

                // TODO: Could Packet just point to ENetPacket instead of copying and destroying?
                // When would ENetPacket get destroyed? Would that confuse ENet in some way?
                enet_packet_destroy(event.packet);

                NetServerClient &client = FindClient(event.peer);
                ProcessMsg(client, packet);

                client.last_packet_received_at = enet_time_get();
                break;
            } case ENET_EVENT_TYPE_DISCONNECT: {
                E_INFO("Client %x:%u disconnected.",
                    event.peer->address.host,
                    event.peer->address.port);
                //TODO: Reset the peer's client information.
                //event.peer->data = NULL;
                auto kv = clients.find(event.peer->address);
                if (kv != clients.end()) {
                    enet_peer_reset(event.peer);
                    memset(&kv->second, 0, sizeof(kv->second));
                    clients.erase(event.peer->address);
                }
                break;
            } case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                E_WARN("Connection timed out for client %x:%hu.",
                    event.peer->address.host,
                    event.peer->address.port);
                auto kv = clients.find(event.peer->address);
                if (kv != clients.end()) {
                    enet_peer_reset(event.peer);
                    memset(&kv->second, 0, sizeof(kv->second));
                    clients.erase(event.peer->address);
                }
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
    clients.clear();
}