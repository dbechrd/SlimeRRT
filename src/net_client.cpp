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

NetClient::NetClient(void)
{
    rawPacket.dataLength = PACKET_SIZE_MAX;
    rawPacket.data = calloc(rawPacket.dataLength, sizeof(uint8_t));
}

NetClient::~NetClient(void)
{
    CloseSocket();
    free(rawPacket.data);
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

#if _DEBUG && CL_DEBUG_REALLY_LONG_TIMEOUT
    uint32_t thirtyMins = 30 * 60 * 1000;
    enet_peer_timeout(server, 32, thirtyMins, thirtyMins);  // long timeout for debugging
#else
    enet_peer_timeout(server, 32, 5000, 15000);  // 32 RTT if > 5s, else if > 15s
#endif
    enet_host_flush(client);

    this->serverHostLength = strlen(serverHost);
    strncpy(this->serverHost, serverHost, SV_HOSTNAME_LENGTH_MAX);
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
    assert(data);
    assert(size);
    assert(size <= PACKET_SIZE_MAX);

    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. Data not sent.");
        return ErrorType::NotConnected;
    }

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
    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. NetMessage not sent.");
        return ErrorType::NotConnected;
    }

    message.connectionToken = connectionToken;
    memset(rawPacket.data, 0, rawPacket.dataLength);
    size_t bytes = message.Serialize(rawPacket);
    //E_INFO("[SEND][%21s][%5u b] %16s ", rawPacket.dataLength, netMsg.TypeString());
    E_ASSERT(SendRaw(rawPacket.data, bytes), "Failed to send packet");
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
    assert(message);

    // TODO: Account for header size, determine MTU we want to aim for, and perhaps do auto-segmentation somewhere
    assert(messageLength);
    assert(messageLength <= CHATMSG_LENGTH_MAX);
    size_t messageLengthSafe = MIN(messageLength, CHATMSG_LENGTH_MAX);

    // If we don't have a username yet (salt, client type, etc.) then we're not connected and can't send chat messages!
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

ErrorType NetClient::SendSlotClick(int slot, bool doubleClicked)
{
    assert(slot >= 0);
    assert(slot < PlayerInvSlot_Count);

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::SlotClick;
    tempMsg.data.slotClick.slotId = slot;
    tempMsg.data.slotClick.doubleClick = doubleClicked;
    ErrorType result = SendMsg(tempMsg);
    return result;
}

ErrorType NetClient::SendSlotScroll(int slot, int scrollY)
{
    assert(slot >= 0);
    assert(slot < PlayerInvSlot_Count);

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::SlotScroll;
    tempMsg.data.slotScroll.slotId = slot;
    tempMsg.data.slotScroll.scrollY = scrollY;
    ErrorType result = SendMsg(tempMsg);
    return result;
}


ErrorType NetClient::SendSlotDrop(int slot, uint32_t count)
{
    assert(slot >= 0);
    assert(slot < PlayerInvSlot_Count);

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::SlotDrop;
    tempMsg.data.slotDrop.slotId = slot;
    tempMsg.data.slotDrop.count = count;
    ErrorType result = SendMsg(tempMsg);
    return result;
}

ErrorType NetClient::SendPlayerInput(void)
{
    if (!worldHistory.Count() || !inputHistory.Count()) {
        return ErrorType::Success;
    }

    const WorldSnapshot &worldSnapshot = worldHistory.Last();

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::Input;

    uint32_t sampleCount = 0;
    for (size_t i = 0; i < inputHistory.Count() && sampleCount < CL_INPUT_SAMPLES_MAX; i++) {
        const InputSample &inputSample = inputHistory.At(i);
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
    // Maybe we want to predict other players' movements by broadcasting their input?
    // Doesn't really seem necessary.. idk.
    //player->Update(now, input.dt);
}

void NetClient::ReconcilePlayer(void)
{
    if (!serverWorld || !worldHistory.Count()) {
        // Not connected to server, or no snapshots received yet
        //TraceLog(LOG_WARNING, "Can't reconcile player; no world");
        return;
    }
    Player *player = serverWorld->FindPlayer(serverWorld->playerId);
    assert(player);
    if (!player) {
        // playerId is invalid??
        TraceLog(LOG_WARNING, "Can't reconcile player; no player found");
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
        // Server sent us a snapshot that doesn't contain our own player??
        TraceLog(LOG_WARNING, "Can't reconcile player; no snapshot");
        return;
    }

    // Client is close enough, don't re-sync to server position
    const Vector3 localPos = player->body.WorldPosition();
    const Vector3 serverPos = playerSnapshot->position;
    if (v3_distance_sq(localPos, serverPos) < SQUARED(METERS_TO_PIXELS(1))) {
        return;
    }
    TraceLog(LOG_DEBUG, "Too far from server pos; reconciling");

    // TODO: Do this more smoothly
    // Roll back local player to server snapshot location
    if (playerSnapshot->flags & PlayerSnapshot::Flags_Position) {
        player->body.Teleport(playerSnapshot->position);
        //TraceLog(LOG_DEBUG, "Teleporting player to %0.2f %0.2f", playerSnapshot->position.x,
        //    playerSnapshot->position.y);

        if (inputHistory.Count()) {
            const InputSample &oldestInput = inputHistory.At(0);
            if (latestSnapshot.lastInputAck + 1 < oldestInput.seq) {
                TraceLog(LOG_WARNING, "inputHistory buffer too small. Server ack'd seq #%u on tick %u, but oldest input we still have is seq #%u",
                    latestSnapshot.lastInputAck, latestSnapshot.tick, oldestInput.seq);
            }
        }
#if CL_DEBUG_PLAYER_RECONCILIATION
        printf("Reconcile player position (tick #%u ack'd input #%u):\n"
            "Teleport: %f %f %f\n", latestSnapshot.tick, latestSnapshot.lastInputAck,
            playerSnapshot->position.x,
            playerSnapshot->position.y,
            playerSnapshot->position.z);
#endif
        //TraceLog(LOG_DEBUG, "Reconcile");
        // Predict player for each input not yet handled by the server
        bool applyOverflow = true;
        for (size_t i = 0; i < inputHistory.Count(); i++) {
            const InputSample &origInput = inputHistory.At(i);
            InputSample input = origInput;
            // NOTE: Old input's ownerId might not match if the player recently reconnected to a
            // server and received a new playerId. Intentionally ignore those.
            if (input.ownerId == player->id && input.seq > latestSnapshot.lastInputAck) {
#if CL_DEBUG_PLAYER_RECONCILIATION
                if (input.walkEast) putchar('>');
                else if (input.walkWest) putchar('<');
                else if (input.walkNorth) putchar('^');
                else if (input.walkSouth) putchar('v');
                else putchar('.');
#endif
                if (applyOverflow) {
                    input.dt = latestSnapshot.inputOverflow;
                    applyOverflow = false;
                }

                //TraceLog(LOG_DEBUG, "CLI SQ: %u OS: %f S: %f", input.seq, origInput.dt, input.dt);
                player->Update(input, serverWorld->map);
            }
        }
#if CL_DEBUG_PLAYER_RECONCILIATION
        const Vector3 after = player->body.WorldPosition();
        putchar('\n');
        printf(
            "Pos: %f\n"
            "     %f\n",
            before.x,
            after.x
        );
        printf("\n");
#endif
    }

    // TODO(dlb): This seems redundant. Doesn't it already happen in ProcessMsg??
    //if (playerSnapshot->flags & PlayerSnapshot::Flags_Direction) {
    //    player->sprite.direction = playerSnapshot->direction;
    //}
    //if (playerSnapshot->flags & PlayerSnapshot::Flags_Speed) {
    //    player->body.speed = playerSnapshot->speed;
    //}
    //if (playerSnapshot->flags & PlayerSnapshot::Flags_Health) {
    //    player->combat.hitPoints = playerSnapshot->hitPoints;
    //}
    //if (playerSnapshot->flags & PlayerSnapshot::Flags_HealthMax) {
    //    player->combat.hitPointsMax = playerSnapshot->hitPointsMax;
    //}
}

void NetClient::ProcessMsg(ENetPacket &packet)
{
    ENetBuffer packetBuffer{ packet.dataLength, packet.data };
    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.Deserialize(packetBuffer);

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

            // TODO: Get tileset ID from server
            serverWorld->map.tilesetId = TilesetID::TS_Overworld;
            //serverWorld->map->GenerateMinimap();
            serverWorld->playerId = welcomeMsg.playerId;

            for (size_t i = 0; i < welcomeMsg.playerCount; i++) {
                NetMessage_Welcome::NetMessage_Welcome_Player &netPlayerInfo = welcomeMsg.players[i];
                PlayerInfo &playerInfo = serverWorld->playerInfos[i];
                playerInfo.id = netPlayerInfo.id;
                playerInfo.nameLength = (uint32_t)MIN(netPlayerInfo.nameLength, USERNAME_LENGTH_MAX);
                memcpy(playerInfo.name, netPlayerInfo.name, playerInfo.nameLength);
            }

            serverWorld->chatHistory.PushServer(welcomeMsg.motd, welcomeMsg.motdLength);
            break;
        } case NetMessage::Type::WorldChunk: {
            NetMessage_WorldChunk &worldChunk = tempMsg.data.worldChunk;
#if CL_DEBUG_WORLD_CHUNKS
            TraceLog(LOG_DEBUG, "Received world chunk %hd %hd", worldChunk.chunk.x, worldChunk.chunk.y);
#endif
            Tilemap &map = serverWorld->map;
            auto chunkIter = map.chunksIndex.find(worldChunk.chunk.Hash());
            if (chunkIter != map.chunksIndex.end()) {
#if CL_DEBUG_WORLD_CHUNKS
                TraceLog(LOG_DEBUG, "  Updating existing chunk");
#endif
                size_t idx = chunkIter->second;
                assert(idx < map.chunks.size());
                map.chunks[chunkIter->second] = worldChunk.chunk;
            } else {
#if CL_DEBUG_WORLD_CHUNKS
                TraceLog(LOG_DEBUG, "  Adding new chunk to chunk list");
#endif
                map.chunks.emplace_back(worldChunk.chunk);
                map.chunksIndex[worldChunk.chunk.Hash()] = map.chunks.size() - 1;
            }
            // TODO(perf): Only update if chunk is within visible region?
            Player *player = serverWorld->FindPlayer(serverWorld->playerId);
            if (player) {
                map.GenerateMinimap(player->body.GroundPosition());
            }
            break;
        } case NetMessage::Type::WorldSnapshot: {
            const WorldSnapshot &netSnapshot = tempMsg.data.worldSnapshot;
            WorldSnapshot &worldSnapshot = worldHistory.Alloc();
            worldSnapshot = netSnapshot;
            worldSnapshot.recvAt = g_clock.now;

            for (size_t i = 0; i < worldSnapshot.playerCount; i++) {
                const PlayerSnapshot &playerSnapshot = worldSnapshot.players[i];
                if (playerSnapshot.flags & PlayerSnapshot::Flags_Despawn) {
                    serverWorld->RemovePlayer(playerSnapshot.id);
                    continue;
                }

                bool spawned = false;
                Player *player = serverWorld->FindPlayer(playerSnapshot.id);
                if (!player) {
                    player = serverWorld->AddPlayer(playerSnapshot.id);
                    if (!player) {
                        continue;
                    }
                    spawned = true;
                }

                if (playerSnapshot.flags != PlayerSnapshot::Flags_None) {
                    //TraceLog(LOG_DEBUG, "Snapshot: player #%u", playerSnapshot.type);
                }

                const bool posChanged = playerSnapshot.flags & PlayerSnapshot::Flags_Position;
                const bool dirChanged = playerSnapshot.flags & PlayerSnapshot::Flags_Direction;

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
                            TraceLog(LOG_WARNING, "Received direction update but prevPosition is not known. playerId: %u", playerSnapshot.id);
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

                if (playerSnapshot.flags & PlayerSnapshot::Flags_Speed) {
                    player->body.speed = playerSnapshot.speed;
                }
                // TODO: Pos/dir are history based, but these are instantaneous.. hmm.. is that okay?
                if (playerSnapshot.flags & PlayerSnapshot::Flags_Health) {
                    //TraceLog(LOG_DEBUG, "Snapshot: health %f", playerSnapshot.hitPoints);
                    if (!spawned) {
                        if (player->combat.hitPoints && !playerSnapshot.hitPoints) {
                            // Died
                            player->combat.diedAt = worldSnapshot.recvAt;
                            ParticleEffectParams bloodParams{};
                            bloodParams.particleCountMin = 128;
                            bloodParams.particleCountMax = bloodParams.particleCountMin;
                            bloodParams.durationMin = 4.0f;
                            bloodParams.durationMax = bloodParams.durationMin;
                            serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Blood, player->WorldCenter(), bloodParams);
                            Catalog::g_sounds.Play(Catalog::SoundID::Eughh, 1.0f + dlb_rand32f_variance(0.1f));
                        } else if (!player->combat.hitPoints && playerSnapshot.hitPoints) {
                            // Respawn
                            player->combat.diedAt = 0;
                        } else if (player->combat.hitPoints > playerSnapshot.hitPoints) {
                            // Took damage
                            Vector3 playerGut = player->GetAttachPoint(Player::AttachPoint::Gut);
                            ParticleEffectParams bloodParams{};
                            bloodParams.particleCountMin = 32;
                            bloodParams.particleCountMax = bloodParams.particleCountMin;
                            bloodParams.durationMin = 1.0f;
                            bloodParams.durationMax = bloodParams.durationMin;
                            ParticleEffect *bloodParticles = serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Blood, playerGut, bloodParams);
                            if (bloodParticles) {
                                bloodParticles->effectCallbacks[(size_t)ParticleEffect_Event::BeforeUpdate] = {
                                    ParticlesFollowPlayerGut,
                                    player
                                };
                            }
                        }
                    }
                    player->combat.hitPoints = playerSnapshot.hitPoints;
                }
                if (playerSnapshot.flags & PlayerSnapshot::Flags_HealthMax) {
                    //TraceLog(LOG_DEBUG, "Snapshot: healthMax %f", playerSnapshot.hitPointsMax);
                    player->combat.hitPointsMax = playerSnapshot.hitPointsMax;
                }
                if (playerSnapshot.flags & PlayerSnapshot::Flags_Level) {
                    //TraceLog(LOG_DEBUG, "Snapshot: level %u", enemySnapshot.level);
                    if (!spawned) {
                        if (playerSnapshot.level && playerSnapshot.level > player->combat.level) {
                            ParticleEffectParams rainbowParams{};
                            rainbowParams.durationMin = 3.0f;
                            rainbowParams.durationMax = rainbowParams.durationMin;
                            rainbowParams.particleCountMin = 256;
                            rainbowParams.particleCountMax = rainbowParams.particleCountMin;
                            ParticleEffect *rainbowFx = serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Rainbow, player->body.WorldPosition(), rainbowParams);
                            if (rainbowFx) {
                                Catalog::g_sounds.Play(Catalog::SoundID::RainbowSparkles, 1.0f);
                            }
                        }
                    }
                    player->combat.level = playerSnapshot.level;
                }
                if (playerSnapshot.flags & PlayerSnapshot::Flags_XP) {
                    player->xp = playerSnapshot.xp;
                }
                if (playerSnapshot.flags & PlayerSnapshot::Flags_Inventory) {
                    player->inventory = playerSnapshot.inventory;
                    //player->inventory.selectedSlot = playerSnapshot.inventory.selectedSlot;
                    //for (size_t i = 0; i < ARRAY_SIZE(playerSnapshot.inventory.slots); i++) {
                    //    player->inventory.slots[i] = playerSnapshot.inventory.slots[i];
                    //}
                }
            }

            for (size_t i = 0; i < worldSnapshot.enemyCount; i++) {
                const EnemySnapshot &enemySnapshot = worldSnapshot.enemies[i];
                if (enemySnapshot.flags & EnemySnapshot::Flags_Despawn) {
                    serverWorld->RemoveSlime(enemySnapshot.id);
                    continue;
                }

                bool spawned = false;
                Slime *slime = serverWorld->FindSlime(enemySnapshot.id);
                if (!slime) {
                    slime = serverWorld->SpawnSlime(enemySnapshot.id, {0, 0});
                    if (!slime) {
                        continue;
                    }
                    spawned = true;
                }

                const bool posChanged = enemySnapshot.flags & EnemySnapshot::Flags_Position;
                const bool dirChanged = enemySnapshot.flags & EnemySnapshot::Flags_Direction;

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

                        if (prevState && prevState->v.z == 0.0f && state.v.z != 0.0f) {
                            //Catalog::SoundID squish = dlb_rand32i_range(0, 1) ? Catalog::SoundID::Squish1 : Catalog::SoundID::Squish2;
                            Catalog::SoundID squish = Catalog::SoundID::Squish1;
                            Catalog::g_sounds.Play(squish, 1.0f + dlb_rand32f_variance(0.2f), true);
                        }
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
                if (enemySnapshot.flags & EnemySnapshot::Flags_Scale) {
                    //TraceLog(LOG_DEBUG, "Snapshot: scale %f", (char)enemySnapshot.direction);
                    slime->sprite.scale = enemySnapshot.scale;
                }
                if (enemySnapshot.flags & EnemySnapshot::Flags_Health) {
                    //TraceLog(LOG_DEBUG, "Snapshot: health %f", enemySnapshot.hitPoints);
                    if (!spawned) {
                        if (slime->combat.hitPoints && !enemySnapshot.hitPoints) {
                            // Died
                            slime->combat.diedAt = worldSnapshot.recvAt;
                            ParticleEffectParams gooParams{};
                            gooParams.particleCountMin = 20;
                            gooParams.particleCountMax = gooParams.particleCountMin;
                            gooParams.durationMin = 2.0f;
                            gooParams.durationMax = gooParams.durationMin;
                            serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Goo, slime->WorldCenter(), gooParams);
                            Catalog::g_sounds.Play(Catalog::SoundID::Squish2, 0.5f + dlb_rand32f_variance(0.1f), true);
                        } else if (slime->combat.hitPoints > enemySnapshot.hitPoints) {
                            // Took damage
                            ParticleEffectParams gooParams{};
                            gooParams.particleCountMin = 5;
                            gooParams.particleCountMax = gooParams.particleCountMin;
                            gooParams.durationMin = 0.5f;
                            gooParams.durationMax = gooParams.durationMin;
                            serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Goo, slime->WorldCenter(), gooParams);
                            Catalog::g_sounds.Play(Catalog::SoundID::Slime_Stab1, 1.0f + dlb_rand32f_variance(0.4f));
                        }
                    }
                    slime->combat.hitPoints = enemySnapshot.hitPoints;
                }
                if (enemySnapshot.flags & EnemySnapshot::Flags_HealthMax) {
                    //TraceLog(LOG_DEBUG, "Snapshot: healthMax %f", enemySnapshot.hitPointsMax);
                    slime->combat.hitPointsMax = enemySnapshot.hitPointsMax;
                }
                if (enemySnapshot.flags & EnemySnapshot::Flags_Level) {
                    //TraceLog(LOG_DEBUG, "Snapshot: level %u", enemySnapshot.level);
                    slime->combat.level = enemySnapshot.level;
                }
            }

            for (size_t i = 0; i < worldSnapshot.itemCount; i++) {
                const ItemSnapshot &itemSnapshot = worldSnapshot.items[i];
                if (itemSnapshot.flags & ItemSnapshot::Flags_Despawn) {
                    serverWorld->itemSystem.Remove(itemSnapshot.id);
                    continue;
                }

                bool spawned = false;
                ItemWorld *item = serverWorld->itemSystem.Find(itemSnapshot.id);
                if (!item) {
#if CL_DEBUG_WORLD_ITEMS
                    TraceLog(LOG_DEBUG, "Trying to spawn item: item #%u, catalog #%u, count %u",
                        itemSnapshot.type,
                        itemSnapshot.catalogId,
                        itemSnapshot.stackCount);
#endif
                    item = serverWorld->itemSystem.SpawnItem(
                        itemSnapshot.position,
                        itemSnapshot.catalogId,
                        itemSnapshot.stackCount,
                        itemSnapshot.id
                    );
                    if (!item) {
                        TraceLog(LOG_ERROR, "Failed to spawn item.");
                        continue;
                    }
                    spawned = true;
                }
#if CL_DEBUG_WORLD_ITEMS
                if (itemSnapshot.flags != ItemSnapshot::Flags_None) {
                    TraceLog(LOG_DEBUG, "Snapshot: item #%u", itemSnapshot.type);
                }
#endif
                const bool posChanged = itemSnapshot.flags & ItemSnapshot::Flags_Position;

                if (posChanged) {
                    const Vector3Snapshot *prevState{};
                    if (item->body.positionHistory.Count()) {
                        prevState = &item->body.positionHistory.Last();
                    }

                    Vector3Snapshot &state = item->body.positionHistory.Alloc();
                    state.recvAt = worldSnapshot.recvAt;

                    if (posChanged) {
                        //TraceLog(LOG_DEBUG, "Snapshot: pos %f %f %f",
                        //    itemSnapshot.position.x,
                        //    itemSnapshot.position.y,
                        //    itemSnapshot.position.z);
                        state.v = itemSnapshot.position;
                    } else {
                        if (prevState) {
                            state.v = prevState->v;
                        } else {
                            TraceLog(LOG_WARNING, "Received direction update but prevPosition is not known.");
                            state.v = item->body.WorldPosition();
                        }
                    }
                }

                //if (itemSnapshot.flags & ItemSnapshot::Flags_Position) {
                //    item->body.Teleport(itemSnapshot.position);
                //}
                if (itemSnapshot.flags & ItemSnapshot::Flags_CatalogId) {
                    item->stack.itemType = itemSnapshot.catalogId;
                }
                if (itemSnapshot.flags & ItemSnapshot::Flags_StackCount) {
                    item->stack.count = itemSnapshot.stackCount;
                    if (!spawned) {
                        if (!item->stack.count && !item->pickedUpAt) {
                            item->pickedUpAt = worldSnapshot.recvAt;
                            Catalog::g_sounds.Play(Catalog::SoundID::Gold, 1.0f + dlb_rand32f_variance(0.2f), true);
                        }
                    }
#if CL_DEBUG_WORLD_ITEMS
                    TraceLog(LOG_DEBUG, "Snapshot: item #%u stack count = %u", item->type, item->stack.count);
#endif
                }
            }
            break;
        } case NetMessage::Type::GlobalEvent: {
            const NetMessage_GlobalEvent &globalEvent = tempMsg.data.globalEvent;

            switch (globalEvent.type) {
                case NetMessage_GlobalEvent::Type::PlayerJoin: {
                    //const NetMessage_GlobalEvent::PlayerJoin &joinEvent = globalEvent.data.playerJoin;
                    //serverWorld->AddPlayerInfo(joinEvent.name, joinEvent.nameLength, joinEvent.playerId);
                    break;
                } case NetMessage_GlobalEvent::Type::PlayerLeave: {
                    assert(!"Deprecated");
                    break;
                }
            }
            break;
        } case NetMessage::Type::NearbyEvent: {
            const NetMessage_NearbyEvent &nearbyEvent = tempMsg.data.nearbyEvent;

            switch (nearbyEvent.type) {
                case NetMessage_NearbyEvent::Type::PlayerState: {
                    assert(!"Deprecated");
                    break;
                } case NetMessage_NearbyEvent::Type::EnemyState: {
                    assert(!"Deprecated");
                    break;
                }
            }
            break;
        } default: {
            E_INFO("Unexpected netMsg itemClass: %s", tempMsg.TypeString());
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

        thread_local const char *prevState = 0;
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
                    assert(!"unhandled event itemClass");
                }
            }
        }
    } while (svc > 0);

    return ErrorType::Success;
}

bool NetClient::IsConnecting(void) const
{
    return server &&
        (server->state >= ENET_PEER_STATE_CONNECTING && server->state < ENET_PEER_STATE_CONNECTED);
}

bool NetClient::IsConnected(void) const
{
    return server && server->state == ENET_PEER_STATE_CONNECTED;
}

bool NetClient::ConnectedAndSpawned(void) const
{
    bool connectedAndSpawned =
        IsConnected() &&
        serverWorld &&
        serverWorld->playerId &&
        serverWorld->FindPlayer(serverWorld->playerId);
    return connectedAndSpawned;
}

bool NetClient::IsDisconnected(void) const
{
    return !server || server->state == ENET_PEER_STATE_DISCONNECTED;
}

void NetClient::Disconnect(void)
{
    memset(serverHost, 0, sizeof(serverHost));
    serverHostLength = 0;
    serverPort = 0;
    memset(username, 0, sizeof(username));
    usernameLength = 0;
    memset(password, 0, sizeof(password));
    passwordLength = 0;

    connectionToken = 0;
    if (server) {
        enet_peer_disconnect_now(server, 1);
        server = nullptr;
    }
    if (serverWorld) {
        delete serverWorld;
        serverWorld = nullptr;
    }
    inputSeq = 0;
    inputHistory.Clear();
    worldHistory.Clear();
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