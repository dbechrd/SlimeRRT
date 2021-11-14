#include "net_client.h"
#include "bit_stream.h"
#include "chat.h"
#include "world.h"
#include "raylib/raylib.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

const char *NetClient::LOG_SRC = "NetClient";

NetClient::~NetClient()
{
    CloseSocket();
}

ErrorType NetClient::OpenSocket()
{
    client = enet_host_create(nullptr, 1, 1, 0, 0);
    if (!client) {
        E_ASSERT(ErrorType::HostCreateFailed, "Failed to create host.");
    }
    // TODO(dlb)[cleanup]: This probably isn't providing any additional value on top of if (!client) check
    assert(client->socket);
    return ErrorType::Success;
}

#pragma warning(push)
#pragma warning(disable:4458)  // parameter hides class member
#pragma warning(disable:4996)  // enet_address_set_host_old deprecated
#pragma warning(disable:6387)  // memcpy pointer could be zero
ErrorType NetClient::Connect(const char *serverHost, unsigned short serverPort, const char *username, const char *password)
{
    ENetAddress address{};

    if (!client) {
        E_ASSERT(OpenSocket(), "Failed to open socket");
    }
    if (server) {
        Disconnect();
    }

    enet_address_set_host(&address, serverHost);
    address.port = serverPort;
    server = enet_host_connect(client, &address, 1, 0);
    assert(server);

    enet_peer_timeout(server, 32, 3000, 5000);
    enet_host_flush(client);

    this->serverHost = serverHost;
    this->serverPort = serverPort;
    this->usernameLength = strlen(username);
    strncpy(this->username, username, USERNAME_LENGTH_MAX);
    this->passwordLength = strlen(password);
    strncpy(this->password, password, PASSWORD_LENGTH_MAX);

    return ErrorType::Success;
}
#pragma warning(pop)

ErrorType NetClient::Auth()
{
    assert(username);
    assert(password);

    NetMessage userIdent{};
    userIdent.type = NetMessage::Type::Identify;
    userIdent.data.identify.usernameLength = (uint32_t)usernameLength;
    memcpy(userIdent.data.identify.username, username, usernameLength);
    userIdent.data.identify.passwordLength = (uint32_t)passwordLength;
    memcpy(userIdent.data.identify.password, password, passwordLength);

    ENetBuffer rawPacket = userIdent.Serialize();
    ENetPacket *packet = enet_packet_create(rawPacket.data, rawPacket.dataLength, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
        E_ASSERT(ErrorType::PacketCreateFailed, "Failed to create packet.");
    }
    if (enet_peer_send(server, 0, packet) < 0) {
        E_ASSERT(ErrorType::PeerSendFailed, "Failed to send connection request.");
    }

    // Clear password from memory
    memset(password, 0, sizeof(password));
    passwordLength = 0;
    return ErrorType::Success;
}

ErrorType NetClient::Send(const void *data, size_t size)
{
    assert(server);
    assert(server->address.port);
    assert(data);
    assert(size);
    assert(size <= PACKET_SIZE_MAX);

    // TODO(dlb): Don't always use reliable flag.. figure out what actually needs to be reliable (e.g. chat)
    ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
        E_ASSERT(ErrorType::PacketCreateFailed, "Failed to create packet.");
    }
    if (enet_peer_send(server, 0, packet) < 0) {
        E_ASSERT(ErrorType::PeerSendFailed, "Failed to send connection request.");
    }
    return ErrorType::Success;
}

ErrorType NetClient::SendChatMessage(const char *message, size_t messageLength)
{
    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. Chat message not sent.");
        chatHistory.PushMessage(CSTR("Sam"), CSTR("You're not connected to a server. Nobody is listening. :("));
        return ErrorType::NotConnected;
    }

    assert(server);
    assert(server->address.port);
    assert(message);

    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(messageLength);
    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    size_t messageLengthSafe = MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX);

    // If we don't have a username yet (salt, client id, etc.) then we're not connected and can't send chat messages!
    // This would be weird since if we're not connected how do we see the chat box?

    NetMessage netMessage{};
    netMessage.type = NetMessage::Type::ChatMessage;
    netMessage.data.chatMsg.usernameLength = (uint32_t)usernameLength;
    memcpy(netMessage.data.chatMsg.username, username, usernameLength);
    netMessage.data.chatMsg.messageLength = (uint32_t)messageLengthSafe;
    memcpy(netMessage.data.chatMsg.message, message, messageLengthSafe);

    ENetBuffer rawPacket = netMessage.Serialize();
    ErrorType result = Send(rawPacket.data, rawPacket.dataLength);
    return result;
}

void NetClient::ProcessMsg(Packet &packet)
{
    packet.netMessage.Deserialize(packet.rawBytes);

    switch (packet.netMessage.type) {
        case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = packet.netMessage.data.chatMsg;
            memcpy(chatMsg.timestampStr, packet.timestampStr, sizeof(packet.timestampStr));
            chatHistory.PushNetMessage(chatMsg);
            break;
        } case NetMessage::Type::Welcome: {
            // TODO: Auth challenge. Store salt sent from server instead.. handshake stuffs
            NetMessage_Welcome &welcomeMsg = packet.netMessage.data.welcome;
            chatHistory.PushMessage(CSTR("Message of the day"), welcomeMsg.motd, welcomeMsg.motdLength);

            // TODO: Use username (ensure null terminated or add player.nameLength field
            delete serverWorld;
            serverWorld = new World;

            serverWorld->map = serverWorld->mapSystem.Generate(serverWorld->rtt_rand, welcomeMsg.width, welcomeMsg.height);
            assert(serverWorld->map);
            // TODO: Get tileset ID from server
            serverWorld->map->tilesetId = TilesetID::TS_Overworld;

            if (!serverWorld->players[0].combat.maxHitPoints) {
                Player *player = serverWorld->SpawnPlayer(username, usernameLength);
                assert(player);
            }

            break;
        } case NetMessage::Type::WorldChunk: {
            NetMessage_WorldChunk &worldChunk = packet.netMessage.data.worldChunk;
            if (worldChunk.tilesLength) {
                serverWorld->map->SyncTiles(worldChunk.tiles, worldChunk.tilesLength);
                serverWorld->players[0].body.position = serverWorld->GetWorldSpawn();
            }
            break;
        } case NetMessage::Type::WorldPlayers: {
            NetMessage_WorldPlayers &worldPlayers = packet.netMessage.data.worldPlayers;
            if (worldPlayers.playersLength) {
                serverWorld->SyncPlayers(worldPlayers.players, worldPlayers.playersLength);
            }
            break;
        } case NetMessage::Type::WorldEntities: {
            NetMessage_WorldEntities &worldEntities = packet.netMessage.data.worldEntities;
            if (worldEntities.entitiesLength) {
                serverWorld->SyncEntities(worldEntities.entities, worldEntities.entitiesLength);
            }
            break;
        } default: {
            E_WARN("Unrecognized message type: %d", packet.netMessage.type);
            break;
        }
    }
}

const char *NetClient::ServerStateString()
{
    if (!server)                                        return "UNITIALIZED                             ";
    switch (server->state) {
        case ENET_PEER_STATE_DISCONNECTED             : return "ENET_PEER_STATE_DISCONNECTED            ";
        case ENET_PEER_STATE_CONNECTING               : return "ENET_PEER_STATE_CONNECTING              ";
        case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT    : return "ENET_PEER_STATE_ACKNOWLEDGING_CONNECT   ";
        case ENET_PEER_STATE_CONNECTION_PENDING       : return "ENET_PEER_STATE_CONNECTION_PENDING      ";
        case ENET_PEER_STATE_CONNECTION_SUCCEEDED     : return "ENET_PEER_STATE_CONNECTION_SUCCEEDED    ";
        case ENET_PEER_STATE_CONNECTED                : return "ENET_PEER_STATE_CONNECTED               ";
        case ENET_PEER_STATE_DISCONNECT_LATER         : return "ENET_PEER_STATE_DISCONNECT_LATER        ";
        case ENET_PEER_STATE_DISCONNECTING            : return "ENET_PEER_STATE_DISCONNECTING           ";
        case ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT : return "ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT";
        case ENET_PEER_STATE_ZOMBIE                   : return "ENET_PEER_STATE_ZOMBIE                  ";
        default                                       : return "IMPOSSIBLE                              ";
    }
}

ErrorType NetClient::Receive()
{
    if (!server) {
        return ErrorType::Success;
    }

    assert(server->address.port);

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?
    int svc = 0;
    do {
        ENetEvent event{};
        svc = enet_host_service(client, &event, 1);

        if (server->state == ENET_PEER_STATE_CONNECTING &&
            !server->lastReceiveTime &&
            (enet_time_get() - server->lastSendTime) > 5000)
        {
            E_WARN("Failed to connect to server %s:%hu.", serverHost, server->host->address.port);
            enet_peer_reset(server);
            //E_ASSERT(ErrorType::PeerConnectFailed, "Failed to connect to server %s:%hu.", hostname, port);
        }

        static const char *prevState = 0;
        const char *curState = ServerStateString();
        if (curState != prevState) {
            E_INFO("%s", curState);
            prevState = curState;
        }

        if (svc > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    E_INFO("Connected to server %x:%hu.",
                        event.peer->address.host,
                        event.peer->address.port);

                    chatHistory.PushMessage(CSTR("Debug"), CSTR("Connected to server. :)"));
                    Auth();

                    break;
                } case ENET_EVENT_TYPE_RECEIVE: {
                    E_INFO("A packet of length %u was received from %x:%u on channel %u.",
                        event.packet->dataLength,
                        event.peer->address.host,
                        event.peer->address.port,
                        event.channelID);

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

                    ProcessMsg(packet);
                    //TraceLog(LOG_INFO, "[NetClient] RECV\n  %s said %s", senderStr, packet.rawBytes);

                    break;
                } case ENET_EVENT_TYPE_DISCONNECT: {
                    E_INFO("Disconnected from server %x:%u.",
                        event.peer->address.host,
                        event.peer->address.port);
                    //TODO: Reset the peer's client information.
                    //event.peer->data = NULL;
                    enet_peer_reset(server);
                    server = nullptr;
                    break;
                } case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    E_WARN("Connection timed out for server %x:%hu.",
                        event.peer->address.host,
                        event.peer->address.port);
                    enet_peer_reset(server);

                    chatHistory.PushMessage(CSTR("Sam"), CSTR("Your connection to the server timed out. :("));
                    break;
                } default: {
                    assert(!"unhandled event type");
                }
            }
        }
    } while (svc > 0);

    return ErrorType::Success;
}

void NetClient::Disconnect()
{
    if (!server) return;

    enet_peer_disconnect(server, 1);
    enet_peer_reset(server);
    server = nullptr;
}

void NetClient::CloseSocket()
{
    Disconnect();
    if (client) {
        enet_host_service(client, nullptr, 0);
        enet_host_destroy(client);
    }
}