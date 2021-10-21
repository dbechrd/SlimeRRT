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
E_START
    ENetAddress address{};
    //address.host = ENET_HOST_ANY;
    address.host = enet_v4_localhost;
    address.port = socketPort;

    server = enet_host_create(&address, 32, 1, 0, 0);
    if (!server) {
        E_FATAL(ErrorType::HostCreateFailed, "Failed to create host.");
    }
    // TODO(dlb)[cleanup]: This probably isn't providing any additional value on top of if (!server) check
    assert(server->socket);
E_CLEAN_END
}

ErrorType NetServer::SendRaw(const NetServerClient *client, const char *data, size_t size)
{
E_START
    assert(client);
    assert(client->peer);
    assert(client->peer->address.port);
    assert(data);
    assert(size <= PACKET_SIZE_MAX);

    // TODO(dlb): Don't always use reliable flag.. figure out what actually needs to be reliable (e.g. chat)
    ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
        E_FATAL(ErrorType::PacketCreateFailed, "Failed to create packet.");
    }
    if (enet_peer_send(client->peer, 0, packet) < 0) {
        E_FATAL(ErrorType::PeerSendFailed, "Failed to send connection request.");
    }
E_CLEAN_END
}

ErrorType NetServer::SendMsg(const NetServerClient *client, const NetMessage &message)
{
E_START
    static char rawPacket[PACKET_SIZE_MAX] = {};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    E_CHECK(SendRaw(client, rawPacket, rawBytes), "Failed to send packet");
E_CLEAN_END
}

ErrorType NetServer::BroadcastRaw(const char *data, size_t size)
{
    ErrorType err_code = ErrorType::Success;

    E_INFO("BROADCAST\n  %.*s", size, data);

    // Broadcast message to all connected clients
    for (auto &kv : clients) {
        assert(kv.second.peer);
        assert(kv.second.peer->address.port);
        ErrorType code = SendRaw(&kv.second, data, size);
        if (code != ErrorType::Success) {
            TraceLog(LOG_ERROR, "[NetServer] BROADCAST\n  %.*s", size, data);
            err_code = code;
            // TODO: Handle error somehow? Retry queue.. or..? Idk.. This seems fatal. Look up why Win32 might fail
        }
    }

    return err_code;
}

ErrorType NetServer::BroadcastMsg(const NetMessage &message)
{
    static char rawPacket[PACKET_SIZE_MAX]{};
    memset(rawPacket, 0, sizeof(rawPacket));

    size_t rawBytes = message.Serialize((uint32_t *)rawPacket, sizeof(rawPacket));
    return BroadcastRaw(rawPacket, rawBytes);
}

ErrorType NetServer::BroadcastChatMessage(const char *msg, size_t msgLength)
{
    NetMessage_ChatMessage netMsg{};
    netMsg.username = SERVER_USERNAME;
    netMsg.usernameLength = sizeof(SERVER_USERNAME) - 1;
    netMsg.message = msg;
    netMsg.messageLength = msgLength;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendWelcomeBasket(NetServerClient *client)
{
E_START
    // TODO: Send current state to new client
    // - world (seed + entities)
    // - player list

    {
        E_INFO("Sending welcome basket to %s\n", TextFormatIP(client->peer->address));
        NetMessage_Welcome userJoinedNotification{};
        E_CHECK(SendMsg(client, userJoinedNotification), "Failed to send welcome basket");
    }

    {
        // TODO: Save from identify packet into NetServerClient, then user client->username
        const char *username = "username";
        size_t usernameLength = strlen(username);
        const char *message = TextFormat("%.*s joined the game.", usernameLength, username);
        size_t messageLength = strlen(message);

        NetMessage_ChatMessage userJoinedNotification{};
        userJoinedNotification.username = "SERVER";
        userJoinedNotification.usernameLength = sizeof("SERVER") - 1;
        userJoinedNotification.messageLength = messageLength;
        userJoinedNotification.message = message;
        E_CHECK(BroadcastMsg(userJoinedNotification), "Failed to send join notification");
    }
E_CLEAN_END
}

void NetServer::ProcessMsg(NetServerClient *client, Packet &packet)
{
    packet.netMessage = &NetMessage::Deserialize((uint32_t *)packet.rawBytes.data, packet.rawBytes.dataLength);

    switch (packet.netMessage->type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = static_cast<NetMessage_Identify &>(*packet.netMessage);
            client->usernameLength = MIN(identMsg.usernameLength, USERNAME_LENGTH_MAX);
            memcpy(client->username, identMsg.username, client->usernameLength);
            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = static_cast<NetMessage_ChatMessage &>(*packet.netMessage);

            // TODO(security): Validate some session token that's not known to other people to prevent impersonation
            //assert(chatMsg.usernameLength == client->usernameLength);
            //assert(!strncmp(chatMsg.username, client->username, client->usernameLength));

            // Store chat message in chat history
            chatHistory.PushNetMessage(chatMsg);

            // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
            // Broadcast chat message
            BroadcastMsg(chatMsg);
            break;
        }
        default: {
            break;
        }
    }
}

ErrorType NetServer::Listen()
{
    assert(server->address.port);

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?

    // TODO(dlb): How long should this wait between calls?
    int svc = 0;
    do {
        ENetEvent event{};
        svc = enet_host_service(server, &event, 50);
        if (svc > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    E_INFO("A new client connected from %x:%u.\n",
                        event.peer->address.host,
                        event.peer->address.port);
                    // TODO: Store any relevant client information here.
                    //event.peer->data = "Client information";
                    break;
                } case ENET_EVENT_TYPE_RECEIVE: {
                    E_INFO("A packet of length %u containing %s was received from %s on channel %u.\n",
                        event.packet->dataLength,
                        event.packet->data,
                        event.peer->data,
                        event.channelID);

                    Packet &packet = packetHistory.Alloc();
                    packet.srcAddress = event.peer->address;
                    packet.timestampStr[0] = '1';  // TODO: Real timestamp? How to get from ENet?
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

                    NetServerClient *client = 0;
                    auto kv = clients.find(event.peer->address);
                    if (kv == clients.end()) {
                        client = &clients[event.peer->address];
                        client->peer = event.peer;
                    } else {
                        client = &kv->second;
                    }

                    if (!client->last_packet_received_at) {
                        SendWelcomeBasket(client);
                    }
                    client->last_packet_received_at = GetTime();

                    ProcessMsg(client, packet);
                    //TraceLog(LOG_INFO, "[NetClient] RECV\n  %s said %s", senderStr, packet.rawBytes);

                    break;

                } case ENET_EVENT_TYPE_DISCONNECT: {
                    E_INFO("%x:%u disconnected.\n",
                        event.peer->address.host,
                        event.peer->address.port);
                    //TODO: Reset the peer's client information.
                    //event.peer->data = NULL;
                } default: {
                    assert(!"unhandled event type");
                }
            }
        }
    } while (svc >= 0);

    return ErrorType::Success;
}

void NetServer::CloseSocket()
{
    // Notify all clients that the server is stopping
    for (int i = 0; i < server->peerCount; i++) {
        enet_peer_disconnect(&server->peers[i], 0);
    }

    enet_host_destroy(server);
    clients.clear();
}