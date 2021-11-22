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
    connectionToken = dlb_rand32u();
    while (!connectionToken) {
        connectionToken = dlb_rand32u();
    }
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

ErrorType NetClient::SendRaw(const void *data, size_t size)
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

ErrorType NetClient::SendMsg(NetMessage &message)
{
    message.connectionToken = connectionToken;
    ENetBuffer rawPacket = message.Serialize(*serverWorld);
    //E_INFO("[SEND][%21s][%5u b] %16s ", rawPacket.dataLength, netMsg.TypeString());
    E_ASSERT(SendRaw(rawPacket.data, rawPacket.dataLength), "Failed to send packet");
    return ErrorType::Success;
}

ErrorType NetClient::Auth()
{
    assert(username);
    assert(password);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::Identify;
    netMsg.data.identify.usernameLength = (uint32_t)usernameLength;
    memcpy(netMsg.data.identify.username, username, usernameLength);
    netMsg.data.identify.passwordLength = (uint32_t)passwordLength;
    memcpy(netMsg.data.identify.password, password, passwordLength);
    ErrorType result = SendMsg(netMsg);

    // Clear password from memory
    memset(password, 0, sizeof(password));
    passwordLength = 0;
    return result;
}

ErrorType NetClient::SendChatMessage(const char *message, size_t messageLength)
{
    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. Chat netMsg not sent.");
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

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::ChatMessage;
    netMsg.data.chatMsg.usernameLength = (uint32_t)usernameLength;
    memcpy(netMsg.data.chatMsg.username, username, usernameLength);
    netMsg.data.chatMsg.messageLength = (uint32_t)messageLengthSafe;
    memcpy(netMsg.data.chatMsg.message, message, messageLengthSafe);
    ErrorType result = SendMsg(netMsg);
    return result;
}

ErrorType NetClient::SendPlayerInput()
{
    ErrorType result = ErrorType::Success;

    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. Input not sent.");
        return ErrorType::NotConnected;
    }
    assert(server);
    assert(server->address.port);

    if (!worldHistory.Count()) {
        return ErrorType::Success;
    }

    WorldSnapshot &worldSnapshot = worldHistory.Last();

    for (size_t i = 0; i < inputHistory.Count(); i++) {
        InputSnapshot &inputSnapshot = inputHistory.At(i);
        if (inputSnapshot.seq > worldSnapshot.inputSeq) {
            // TODO: Wrap multiple inputs into a single packet
            memset(&netMsg, 0, sizeof(netMsg));
            netMsg.type = NetMessage::Type::Input;
            netMsg.data.input = inputHistory.At(i);
            ErrorType sendResult = SendMsg(netMsg);
            if (sendResult != ErrorType::Success) {
                result = sendResult;
            }
        }
    }

    return result;
}

void NetClient::PredictPlayer()
{
    // TODO: What is "now" used for? Where do I get it from that makes sense?
    //double now = glfwGetTime();
    //player->Update(now, input.dt);
}

void NetClient::ReconcilePlayer()
{
    if (!serverWorld || !worldHistory.Count()) {
        // Not connected to server, or no snapshots received yet
        return;
    }
    Player *player = serverWorld->FindPlayer(serverWorld->playerId);
    assert(player);
    if (!player) {
        // playerId is invalid??
        return;
    }

    WorldSnapshot &latestSnapshot = worldHistory.Last();
    PlayerSnapshot *playerSnapshot = 0;
    for (size_t i = 0; i < WORLD_SNAPSHOT_PLAYERS_MAX; i++) {
        if (latestSnapshot.players[i].id == serverWorld->playerId) {
            playerSnapshot = &latestSnapshot.players[i];
            break;
        }
    }
    assert(playerSnapshot);
    if (!playerSnapshot) {
        // Server sent us a snapshot that doesn't contain our own player??
        return;
    }

    // TODO: Do this more smoothly
    // Roll back local player to server snapshot location
    //player->nameLength = playerSnapshot->nameLength;
    //memcpy(player->name, playerSnapshot->name, USERNAME_LENGTH_MAX);
    player->body.acceleration.x = playerSnapshot->acc_x;
    player->body.acceleration.y = playerSnapshot->acc_y;
    player->body.acceleration.z = playerSnapshot->acc_z;
    player->body.velocity.x     = playerSnapshot->vel_x;
    player->body.velocity.y     = playerSnapshot->vel_y;
    player->body.velocity.z     = playerSnapshot->vel_z;
    player->body.prevPosition.x = playerSnapshot->prev_pos_x;
    player->body.prevPosition.y = playerSnapshot->prev_pos_y;
    player->body.prevPosition.z = playerSnapshot->prev_pos_z;
    player->body.position.x     = playerSnapshot->pos_x;
    player->body.position.y     = playerSnapshot->pos_y;
    player->body.position.z     = playerSnapshot->pos_z;
    player->combat.hitPoints    = playerSnapshot->hitPoints;
    player->combat.hitPointsMax = playerSnapshot->hitPointsMax;

    // Predict player for each input not yet handled by the server
    for (size_t i = 0; i < inputHistory.Count(); i++) {
        InputSnapshot &input = inputHistory.At(i);
        // NOTE: Old input's ownerId might not match if the player recently
        // reconnect to a server and received a new playerId
        if (input.ownerId == player->id && input.seq > latestSnapshot.inputSeq) {
            player->Update(input.frameTime, input.frameDt);
        }
    }
}

void NetClient::ProcessMsg(ENetPacket &packet)
{
    ENetBuffer packetBuffer{ packet.dataLength, packet.data };
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.Deserialize(packetBuffer, *serverWorld);

    if (connectionToken && netMsg.connectionToken != connectionToken) {
        // Received a netMsg from a stale connection; discard it
        printf("Ignoring %s packet from stale connection.\n", netMsg.TypeString());
        return;
    }

    switch (netMsg.type) {
        case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = netMsg.data.chatMsg;
            serverWorld->chatHistory.PushNetMessage(chatMsg);
            break;
        } case NetMessage::Type::Welcome: {
            // TODO: Auth challenge. Store salt sent from server instead.. handshake stuffs
            NetMessage_Welcome &welcomeMsg = netMsg.data.welcome;
            serverWorld->chatHistory.PushMessage(CSTR("Message of the day"), welcomeMsg.motd, welcomeMsg.motdLength);

            // TODO: Move this logic to net_message.cpp like all the other logic?
            serverWorld->map = serverWorld->mapSystem.Generate(serverWorld->rtt_rand, welcomeMsg.width, welcomeMsg.height);
            assert(serverWorld->map);
            // TODO: Get tileset ID from server
            serverWorld->map->tilesetId = TilesetID::TS_Overworld;
            serverWorld->playerId = welcomeMsg.playerId;
            break;
        } case NetMessage::Type::WorldChunk: {
            //NetMessage_WorldChunk &worldChunk = netMsg.data.worldChunk;
            break;
        } case NetMessage::Type::WorldSnapshot: {
            WorldSnapshot &netSnapshot = netMsg.data.worldSnapshot;
            WorldSnapshot &worldSnapshot = worldHistory.Alloc();
            worldSnapshot = netSnapshot;
            //memcpy(&worldSnapshot, &netSnapshot, sizeof(historySnapshot));
            break;
        } default: {
            E_WARN("Unexpected netMsg type: %s", netMsg.TypeString());
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

        //if (server &&
        //    server->state == ENET_PEER_STATE_CONNECTING &&
        //    !server->lastReceiveTime &&
        //    (enet_time_get() - server->lastSendTime) > 5000)
        //{
        //    E_WARN("Failed to connect to server %s:%hu.", serverHost, server->host->address.port);
        //    enet_peer_reset(server);
        //    //E_ASSERT(ErrorType::PeerConnectFailed, "Failed to connect to server %s:%hu.", hostname, port);
        //}

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

                    assert(!serverWorld);
                    serverWorld = new World;
                    serverWorld->chatHistory.PushMessage(CSTR("Debug"), CSTR("Connected to server. :)"));
                    Auth();
                    break;
                } case ENET_EVENT_TYPE_RECEIVE: {
                    //E_INFO("A packet of length %u was received from %x:%u on channel %u.",
                    //    event.packet->dataLength,
                    //    event.peer->address.host,
                    //    event.peer->address.port,
                    //    event.channelID);

                    ProcessMsg(*event.packet);
                    enet_packet_destroy(event.packet);
                    //TraceLog(LOG_INFO, "[NetClient] RECV\n  %s said %s", senderStr, packet.rawBytes);
                    break;
                } case ENET_EVENT_TYPE_DISCONNECT: {
                    E_INFO("Disconnected from server %x:%u.",
                        event.peer->address.host,
                        event.peer->address.port);

                    enet_peer_reset(server);
                    server = nullptr;
                    if (serverWorld) {
                        delete serverWorld;
                        serverWorld = nullptr;
                    }
                    inputHistory.Clear();
                    worldHistory.Clear();
                    //serverWorld->chatHistory.PushMessage(CSTR("Sam"), CSTR("Disconnected from server."));
                    break;
                } case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    E_WARN("Connection timed out for server %x:%hu.",
                        event.peer->address.host,
                        event.peer->address.port);

                    enet_peer_reset(server);
                    server = nullptr;
                    if (serverWorld) {
                        delete serverWorld;
                        serverWorld = nullptr;
                    }
                    inputHistory.Clear();
                    worldHistory.Clear();
                    //serverWorld->chatHistory.PushMessage(CSTR("Sam"), CSTR("Your connection to the server timed out. :("));
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
    if (server) {
        enet_peer_disconnect(server, 1);
        enet_host_service(client, nullptr, 0);
        enet_peer_reset(server);
        server = nullptr;
    }
    if (serverWorld) {
        delete serverWorld;
        serverWorld = nullptr;
    }
    inputHistory.Clear();
    worldHistory.Clear();
    connectionToken = 0;
}

void NetClient::CloseSocket()
{
    Disconnect();
    if (client) {
        enet_host_service(client, nullptr, 0);
        enet_host_destroy(client);
        client = nullptr;
    }
}