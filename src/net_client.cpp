#include "net_client.h"
#include "bit_stream.h"
#include "chat.h"
#include "world.h"
#include "raylib/raylib.h"
#include "enet_zpl.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

const char *NetClient::LOG_SRC = "NetClient";

NetClient::~NetClient(void)
{
    CloseSocket();
}

ErrorType NetClient::OpenSocket(void)
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
    ENetBuffer rawPacket{ PACKET_SIZE_MAX, calloc(PACKET_SIZE_MAX, sizeof(uint8_t)) };
    message.Serialize(*serverWorld, rawPacket);
    //E_INFO("[SEND][%21s][%5u b] %16s ", rawPacket.dataLength, netMsg.TypeString());
    E_ASSERT(SendRaw(rawPacket.data, rawPacket.dataLength), "Failed to send packet");
    free(rawPacket.data);
    return ErrorType::Success;
}

ErrorType NetClient::Auth(void)
{
    assert(usernameLength);
    assert(passwordLength);

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::Identify;
    tempMsg.data.identify.usernameLength = (uint32_t)usernameLength;
    memcpy(tempMsg.data.identify.username, username, usernameLength);
    tempMsg.data.identify.passwordLength = (uint32_t)passwordLength;
    memcpy(tempMsg.data.identify.password, password, passwordLength);
    ErrorType result = SendMsg(tempMsg);

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
    assert(messageLength <= CHATMSG_LENGTH_MAX);
    size_t messageLengthSafe = MIN(messageLength, CHATMSG_LENGTH_MAX);

    // If we don't have a username yet (salt, client id, etc.) then we're not connected and can't send chat messages!
    // This would be weird since if we're not connected how do we see the chat box?

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::ChatMessage;
    tempMsg.data.chatMsg.source = NetMessage_ChatMessage::Source::Client;
    tempMsg.data.chatMsg.id = serverWorld->playerId;
    tempMsg.data.chatMsg.messageLength = (uint32_t)messageLengthSafe;
    memcpy(tempMsg.data.chatMsg.message, message, messageLengthSafe);
    ErrorType result = SendMsg(tempMsg);
    return result;
}

ErrorType NetClient::SendPlayerInput(void)
{
    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. Input not sent.");
        return ErrorType::NotConnected;
    }
    assert(server);
    assert(server->address.port);

    if (!worldHistory.Count() || !inputHistory.Count()) {
        return ErrorType::Success;
    }

    const WorldSnapshot &worldSnapshot = worldHistory.Last();

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::Input;

    uint32_t sampleCount = 0;
    for (size_t i = 0; i < inputHistory.Count() && sampleCount < CL_INPUT_SAMPLES_MAX; i++) {
        InputSample &inputSample = inputHistory.At(i);
        if (inputSample.seq > worldSnapshot.lastInputAck) {
            //if (sampleCount == 0) { printf("Sending input seq:"); }
            //printf(" %u", inputSample.seq);
            tempMsg.data.input.samples[sampleCount++] = inputSample;
        }
    }
    //putchar('\n');
    //fflush(stdout);
    tempMsg.data.input.sampleCount = sampleCount;
    return SendMsg(tempMsg);
}

void NetClient::PredictPlayer(void)
{
    // TODO: What is "now" used for? Where do I get it from that makes sense?
    //double now = glfwGetTime();
    //player->Update(now, input.dt);
}

void NetClient::ReconcilePlayer(double tickDt)
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

    const WorldSnapshot &latestSnapshot = worldHistory.Last();
    const PlayerSnapshot *playerSnapshot = 0;
    for (size_t i = 0; i < latestSnapshot.playerCount; i++) {
        if (latestSnapshot.players[i].id == serverWorld->playerId) {
            playerSnapshot = &latestSnapshot.players[i];
            break;
        }
    }
    assert(playerSnapshot);
    if (!playerSnapshot) {
        // Server sent us a slimeSnapshot that doesn't contain our own player??
        return;
    }

    // TODO: Do this more smoothly
    // Roll back local player to server slimeSnapshot location
    if (playerSnapshot->flags & PlayerSnapshot::Flags::Position) {
        player->body.Teleport(playerSnapshot->position);
    }
    if (playerSnapshot->flags & PlayerSnapshot::Flags::Direction) {
        player->sprite.direction = playerSnapshot->direction;
    }
    if (playerSnapshot->flags & PlayerSnapshot::Flags::Health) {
        player->combat.hitPoints = playerSnapshot->hitPoints;
    }
    if (playerSnapshot->flags & PlayerSnapshot::Flags::HealthMax) {
        player->combat.hitPointsMax = playerSnapshot->hitPointsMax;
    }

#if 1
    if (inputHistory.Count()) {
        const InputSample &oldestInput = inputHistory.At(0);
        if (latestSnapshot.lastInputAck + 1 < oldestInput.seq) {
            TraceLog(LOG_WARNING, "inputHistory buffer too small. Server ack'd seq #%u on tick %u, but oldest input we still have is seq #%u",
                latestSnapshot.lastInputAck, latestSnapshot.tick, oldestInput.seq);
        }
    }

    // Predict player for each input not yet handled by the server
    for (size_t i = 0; i < inputHistory.Count(); i++) {
        InputSample &input = inputHistory.At(i);
        // NOTE: Old input's ownerId might not match if the player recently
        // reconnect to a server and received a new playerId
        if (input.ownerId == player->id && input.seq > latestSnapshot.lastInputAck) {
            assert(serverWorld->map);
            player->Update(tickDt, input, *serverWorld->map);
        }
    }
#endif
}

void NetClient::ProcessMsg(ENetPacket &packet)
{
    ENetBuffer packetBuffer{ packet.dataLength, packet.data };
    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.Deserialize(*serverWorld, packetBuffer);

    if (connectionToken && tempMsg.connectionToken != connectionToken) {
        // Received a netMsg from a stale connection; discard it
        printf("Ignoring %s packet from stale connection.\n", tempMsg.TypeString());
        return;
    }

    switch (tempMsg.type) {
        case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = tempMsg.data.chatMsg;
            serverWorld->chatHistory.PushNetMessage(chatMsg);
            break;
        } case NetMessage::Type::Welcome: {
            // TODO: Auth challenge. Store salt sent from server instead.. handshake stuffs
            NetMessage_Welcome &welcomeMsg = tempMsg.data.welcome;
            serverWorld->chatHistory.PushServer(welcomeMsg.motd, welcomeMsg.motdLength);

            serverWorld->map = serverWorld->mapSystem.Generate(serverWorld->rtt_rand, welcomeMsg.width, welcomeMsg.height);
            //serverWorld->map->GenerateMinimap();
            assert(serverWorld->map);
            // TODO: Get tileset ID from server
            serverWorld->map->tilesetId = TilesetID::TS_Overworld;
            serverWorld->playerId = welcomeMsg.playerId;

            for (size_t i = 0; i < welcomeMsg.playerCount; i++) {
                Player *player = serverWorld->AddPlayer(welcomeMsg.players[i].id);
                if (player) {
                    player->nameLength = (uint32_t)MIN(welcomeMsg.players[i].nameLength, USERNAME_LENGTH_MAX);
                    memcpy(player->name, welcomeMsg.players[i].name, player->nameLength);
                }
            }
            break;
        } case NetMessage::Type::WorldChunk: {
            //NetMessage_WorldChunk &worldChunk = netMsg.data.worldChunk;
            break;
        } case NetMessage::Type::WorldSnapshot: {
            const WorldSnapshot &netSnapshot = tempMsg.data.worldSnapshot;
            WorldSnapshot &worldSnapshot = worldHistory.Alloc();
            worldSnapshot = netSnapshot;
            worldSnapshot.recvAt = glfwGetTime();

#if 0
            for (size_t i = 0; i < worldSnapshot.playerCount; i++) {
                const PlayerSnapshot &playerSnapshot = worldSnapshot.players[i];
                Player *player = serverWorld->FindPlayer(playerSnapshot.id);
                if (!player) {
                    continue;
                }

                bool init = playerSnapshot.init || playerSnapshot.spawned;

                // TODO: World::Interpolate() should used snapshots directly.. why did I change that?
                if (playerSnapshot.nearby) {
                    if (playerSnapshot.spawned) {
                        // TODO: Play spawn animation
                    }
                    if (init || playerSnapshot.moved) {
                        Vector3Snapshot &pos = player->body.positionHistory.Alloc();
                        pos.recvAt = worldSnapshot.recvAt;
                        pos.v = playerSnapshot.position;
                        pos.direction = playerSnapshot.direction;
                    }
                    if (init || playerSnapshot.tookDamage || playerSnapshot.healed) {
                        player->combat.hitPoints = playerSnapshot.hitPoints;
                        player->combat.hitPointsMax = playerSnapshot.hitPointsMax;
                    }
                } else {
                    serverWorld->DespawnPlayer(playerSnapshot.id);
                }
            }
#else
            for (size_t i = 0; i < worldSnapshot.playerCount; i++) {
                const PlayerSnapshot &playerSnapshot = worldSnapshot.players[i];
                Player *player = serverWorld->FindPlayer(playerSnapshot.id);
                if (!player) {
                    TraceLog(LOG_ERROR, "Failed to find player %u.", playerSnapshot.id);
                    continue;
                }

                if (playerSnapshot.flags != PlayerSnapshot::Flags::None) {
                    //TraceLog(LOG_DEBUG, "Snapshot: player #%u", playerSnapshot.id);
                }

                const bool posChanged = playerSnapshot.flags & PlayerSnapshot::Flags::Position;
                const bool dirChanged = playerSnapshot.flags & PlayerSnapshot::Flags::Direction;

                if (posChanged || dirChanged) {
                    const Vector3Snapshot *prevState{};
                    if (player->body.positionHistory.Count()) {
                        prevState = &player->body.positionHistory.Last();
                    }

                    Vector3Snapshot &state = player->body.positionHistory.Alloc();
                    state.recvAt = worldSnapshot.recvAt;

                    if (posChanged) {
                        //TraceLog(LOG_DEBUG, "Snapshot: pos %f %f %f",
                            //playerSnapshot.position.x,
                            //playerSnapshot.position.y,
                            //playerSnapshot.position.z);
                        state.v = playerSnapshot.position;
                    } else {
                        if (prevState) {
                            state.v = prevState->v;
                        } else {
                            //TraceLog(LOG_WARNING, "Received direction update but prevPosition is not known.");
                            state.v = player->body.WorldPosition();
                        }
                    }

                    if (dirChanged) {
                        state.direction = playerSnapshot.direction;
                        //TraceLog(LOG_DEBUG, "Snapshot: dir %d", (char)state.direction);
                    } else {
                        if (prevState) {
                            state.direction = prevState->direction;
                            //TraceLog(LOG_DEBUG, "Snapshot: dir %d (fallback prev)", (char)state.direction);
                        } else {
                            TraceLog(LOG_WARNING, "Received position update but prevState.direction is not available.");
                            state.direction = player->sprite.direction;
                        }
                    }
                }

                // TODO: Pos/dir are history based, but these are instantaneous.. hmm.. is that okay?
                if (playerSnapshot.flags & PlayerSnapshot::Flags::Health) {
                    //TraceLog(LOG_DEBUG, "Snapshot: health %f", playerSnapshot.hitPoints);
                    // TODO: Move this to DespawnStaleEntities or whatever
                    if (player->combat.hitPoints && !playerSnapshot.hitPoints) {
                        //TraceLog(LOG_DEBUG, "Snapshot: Player died");
                        player->combat.diedAt = worldSnapshot.recvAt;
                        serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Blood, 20, player->WorldCenter(), 2.0);
                        Catalog::g_sounds.Play(Catalog::SoundID::Slime_Stab1, 0.5f + dlb_rand32f_variance(0.1f), true);
                    }
                    player->combat.hitPoints = playerSnapshot.hitPoints;
                }
                if (playerSnapshot.flags & PlayerSnapshot::Flags::HealthMax) {
                    //TraceLog(LOG_DEBUG, "Snapshot: healthMax %f", playerSnapshot.hitPointsMax);
                    player->combat.hitPointsMax = playerSnapshot.hitPointsMax;
                }
            }
#endif
            for (size_t i = 0; i < worldSnapshot.enemyCount; i++) {
                const EnemySnapshot &enemySnapshot = worldSnapshot.enemies[i];
                Slime *slime = serverWorld->FindSlime(enemySnapshot.id);
                if (!slime) {
                    slime = serverWorld->SpawnSlime(enemySnapshot.id);
                    if (!slime) {
                        TraceLog(LOG_ERROR, "Failed to spawn slime.");
                        continue;
                    }
                }

                if (enemySnapshot.flags != EnemySnapshot::Flags::None) {
                    TraceLog(LOG_DEBUG, "Snapshot: enemy #%u", enemySnapshot.id);
                }

                const bool posChanged = enemySnapshot.flags & EnemySnapshot::Flags::Position;
                const bool dirChanged = enemySnapshot.flags & EnemySnapshot::Flags::Direction;

                if (posChanged || dirChanged) {
                    const Vector3Snapshot *prevState{};
                    if (slime->body.positionHistory.Count()) {
                        prevState = &slime->body.positionHistory.Last();
                    }

                    Vector3Snapshot &state = slime->body.positionHistory.Alloc();
                    state.recvAt = worldSnapshot.recvAt;

                    if (posChanged) {
                        //TraceLog(LOG_DEBUG, "Snapshot: pos %f %f %f",
                        //    enemySnapshot.position.x,
                        //    enemySnapshot.position.y,
                        //    enemySnapshot.position.z);
                        state.v = enemySnapshot.position;
                    } else {
                        if (prevState) {
                            state.v = prevState->v;
                        } else {
                            TraceLog(LOG_WARNING, "Received direction update but prevPosition is not known.");
                            state.v = slime->body.WorldPosition();
                        }
                    }

                    if (dirChanged) {
                        state.direction = enemySnapshot.direction;
                        //TraceLog(LOG_DEBUG, "Snapshot: dir %d", (char)state.direction);
                    } else {
                        if (prevState) {
                            state.direction = prevState->direction;
                            //TraceLog(LOG_DEBUG, "Snapshot: dir %d (fallback prev)", (char)state.direction);
                        } else {
                            TraceLog(LOG_WARNING, "Received position update but prevState.direction is not available.");
                            state.direction = slime->sprite.direction;
                        }
                    }
                }

                // TODO: Pos/dir are history based, but these are instantaneous.. hmm.. is that okay?
                if (enemySnapshot.flags & EnemySnapshot::Flags::Scale) {
                    //TraceLog(LOG_DEBUG, "Snapshot: scale %f", (char)enemySnapshot.direction);
                    slime->sprite.scale = enemySnapshot.scale;
                }
                if (enemySnapshot.flags & EnemySnapshot::Flags::Health) {
                    //TraceLog(LOG_DEBUG, "Snapshot: health %f", enemySnapshot.hitPoints);
                    // TODO: Move this to DespawnStaleEntities or whatever
                    if (slime->combat.hitPoints && !enemySnapshot.hitPoints) {
                        //TraceLog(LOG_DEBUG, "Snapshot: Slime died");
                        slime->combat.diedAt = worldSnapshot.recvAt;
                        serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Goo, 20, slime->WorldCenter(), 2.0);
                        Catalog::g_sounds.Play(Catalog::SoundID::Squish2, 0.5f + dlb_rand32f_variance(0.1f), true);
                    }
                    slime->combat.hitPoints = enemySnapshot.hitPoints;
                }
                if (enemySnapshot.flags & EnemySnapshot::Flags::HealthMax) {
                    //TraceLog(LOG_DEBUG, "Snapshot: healthMax %f", enemySnapshot.hitPointsMax);
                    slime->combat.hitPointsMax = enemySnapshot.hitPointsMax;
                }
            }

            for (size_t i = 0; i < worldSnapshot.itemCount; i++) {
                const ItemSnapshot &itemSnapshot = worldSnapshot.items[i];
                ItemWorld *item = serverWorld->itemSystem.Find(itemSnapshot.id);
                if (!item) {
                    item = serverWorld->itemSystem.SpawnItem(serverWorld->GetWorldSpawn(), Catalog::ItemID::Weapon_Sword, 42, itemSnapshot.id);
                    if (!item) {
                        TraceLog(LOG_ERROR, "Failed to spawn item.");
                        continue;
                    }
                }

                if (itemSnapshot.flags != ItemSnapshot::Flags::None) {
                    TraceLog(LOG_DEBUG, "Snapshot: item #%u", itemSnapshot.id);
                }

                if (itemSnapshot.flags & ItemSnapshot::Flags::Position) {
                    item->body.Teleport(itemSnapshot.position);
                }
                if (itemSnapshot.flags & ItemSnapshot::Flags::CatalogId) {
                    item->stack.id = itemSnapshot.catalogId;
                }
                if (itemSnapshot.flags & ItemSnapshot::Flags::StackCount) {
                    item->stack.count = itemSnapshot.stackCount;
                }
                if (itemSnapshot.flags & ItemSnapshot::Flags::PickedUp) {
                    item->pickedUp = itemSnapshot.pickedUp;
                }
            }
            break;
        } case NetMessage::Type::GlobalEvent: {
            const NetMessage_GlobalEvent &globalEvent = tempMsg.data.globalEvent;

            switch (globalEvent.type) {
                case NetMessage_GlobalEvent::Type::PlayerJoin: {
                    const NetMessage_GlobalEvent::PlayerJoin &playerJoin = globalEvent.data.playerJoin;
                    if (playerJoin.playerId != serverWorld->playerId) {
                        Player *player = serverWorld->AddPlayer(playerJoin.playerId);
                        if (player) {
                            player->nameLength = playerJoin.nameLength;
                            memcpy(player->name, playerJoin.name, player->nameLength);
                        }
                    }
                    break;
                } case NetMessage_GlobalEvent::Type::PlayerLeave: {
                    const NetMessage_GlobalEvent::PlayerLeave &playerLeave = globalEvent.data.playerLeave;
                    serverWorld->RemovePlayer(playerLeave.playerId);
                    break;
                }
            }
            break;
        } case NetMessage::Type::NearbyEvent: {
            const NetMessage_NearbyEvent &nearbyEvent = tempMsg.data.nearbyEvent;

            switch (nearbyEvent.type) {
                case NetMessage_NearbyEvent::Type::PlayerState: {
#if 0
                    const NetMessage_NearbyEvent::PlayerState &state = nearbyEvent.data.playerState;

                    Player *player = serverWorld->FindPlayer(state.id);
                    if (player) {
                        if (state.nearby) {
                            if (state.spawned) {
                                serverWorld->SpawnPlayer(player->id);
                            }
                            if (state.attacked) {
                                player->actionState = Player::ActionState::Attacking;
                            }
                            if (state.moved) {
                                player->body.position = state.position;
                                player->sprite.direction = state.direction;
                            }
                            if (state.tookDamage || state.healed) {
                                player->combat.hitPoints = state.hitPoints;
                                player->combat.hitPointsMax = state.hitPointsMax;
                            }
                        } else {
                            serverWorld->DespawnPlayer(state.id);
                        }
                    } else {
                        TraceLog(LOG_ERROR, "Could not find player to update state. playerId: %u", state.id);
                    }
#else
                    assert(!"Deprecated");
#endif
                    break;
                } case NetMessage_NearbyEvent::Type::EnemyState: {
#if 0
                    const NetMessage_NearbyEvent::EnemyState &state = nearbyEvent.data.enemyState;

                    if (state.nearby) {
                        Slime *enemy = serverWorld->FindSlime(state.id);
                        if (!enemy) {
                            enemy = serverWorld->SpawnSlime(state.id);
                        }
                        if (enemy) {
                            if (state.spawned) {
                                // TODO: Set actionState to spawning and play animation?
                            }
                            if (state.attacked) {
                                enemy->actionState = Slime::ActionState::Attacking;
                            }
                            if (state.moved) {
                                enemy->body.Teleport(state.position);
                                enemy->sprite.direction = state.direction;
                            }
                            if (state.tookDamage || state.healed) {
                                enemy->combat.hitPoints = state.hitPoints;
                                enemy->combat.hitPointsMax = state.hitPointsMax;
                            }
                        } else {
                            TraceLog(LOG_ERROR, "Could not find enemy to update state. enemyId: %u", state.id);
                        }
                    } else {
                        // TODO: Set actionState to despawning and play animation instead?
                        serverWorld->DespawnSlime(state.id);
                    }
#else
                    assert(!"Deprecated");
#endif
                    break;
                }
            }
            break;
        } default: {
            E_INFO("Unexpected netMsg type: %s", tempMsg.TypeString());
            break;
        }
    }
}

const char *NetClient::ServerStateString(void)
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

ErrorType NetClient::Receive(void)
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
                    serverWorld->chatHistory.PushDebug(CSTR("Connected to server. :)"));
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

                    Disconnect();
                    //serverWorld->chatHistory.PushMessage(CSTR("Sam"), CSTR("Disconnected from server."));
                    break;
                } case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    E_WARN("Connection timed out for server %x:%hu.",
                        event.peer->address.host,
                        event.peer->address.port);

                    Disconnect();
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

bool NetClient::IsConnecting(void)
{
    return server &&
        (server->state >= ENET_PEER_STATE_CONNECTING && server->state < ENET_PEER_STATE_CONNECTED);
}

bool NetClient::IsConnected(void)
{
    return server && server->state == ENET_PEER_STATE_CONNECTED;
}

bool NetClient::IsDisconnected(void)
{
    return !server || server->state == ENET_PEER_STATE_DISCONNECTED;
}

void NetClient::Disconnect(void)
{
    if (server) {
        enet_peer_disconnect_now(server, 1);
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

void NetClient::CloseSocket(void)
{
    Disconnect();
    if (client) {
        enet_host_service(client, nullptr, 0);
        enet_host_destroy(client);
        client = nullptr;
    }
}