#include "net_client.h"
#include "bit_stream.h"
#include "chat.h"
#include "servers_generated.h"
#include "world.h"
#include "raylib/raylib.h"
#include "enet_zpl.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>

uint8_t NetClient::rawPacket[PACKET_SIZE_MAX];

ErrorType NetClient::SaveDefaultServerDB(const char *filename)
{
    DB::ServerT tpjGuest{};
    tpjGuest.desc = "TPJ Guest";
    tpjGuest.host = "slime.theprogrammingjunkie.com";
    tpjGuest.port = SV_DEFAULT_PORT;
    tpjGuest.user = "guest";
    tpjGuest.pass = "guest";
    E_ERROR_RETURN(server_db.Add(tpjGuest), "Failed to add default server to ServerDB", 0);
    E_ERROR_RETURN(server_db.Save(filename), "Failed to save default ServerDB", 0);
    return ErrorType::Success;
}

ErrorType NetClient::Load(void)
{
    if (server_db.Load("db/servers.dat") == ErrorType::FileReadFailed) {
        E_ERROR_RETURN(SaveDefaultServerDB("db/servers.dat"), "Failed to save default server DB", 0);
    };

    //rawPacket.dataLength = PACKET_SIZE_MAX;
    //rawPacket.data = calloc(rawPacket.dataLength, sizeof(uint8_t));

    return ErrorType::Success;
}

NetClient::~NetClient(void)
{
    CloseSocket();
    //free(rawPacket.data);
}

ErrorType NetClient::OpenSocket(void)
{
    connectionToken = dlb_rand32u();
    while (!connectionToken) {
        connectionToken = dlb_rand32u();
    }
    client = enet_host_create(nullptr, 1, 1, 0, 0);
    if (!client) {
        E_ERROR_RETURN(ErrorType::HostCreateFailed, "Failed to create host.", 0);
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
        E_ERROR_RETURN(OpenSocket(), "Failed to open socket", 0);
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
    strncpy(this->serverHost, serverHost, HOSTNAME_LENGTH_MAX);
    this->serverPort = serverPort;
    this->usernameLength = strlen(username);
    strncpy(this->username, username, USERNAME_LENGTH_MAX);
    this->passwordLength = strlen(password);
    strncpy(this->password, password, PASSWORD_LENGTH_MAX);

    return ErrorType::Success;
}
#pragma warning(pop)

ErrorType NetClient::SendRaw(const uint8_t *buf, size_t len)
{
    assert(buf);
    assert(len);
    assert(len <= PACKET_SIZE_MAX);

    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. Data not sent.", 0);
        return ErrorType::NotConnected;
    }

    // TODO(dlb): Don't always use reliable flag.. figure out what actually needs to be reliable (e.g. chat)
    ENetPacket *packet = enet_packet_create(buf, len, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
        E_ERROR_RETURN(ErrorType::PacketCreateFailed, "Failed to create packet.", 0);
    }
    if (enet_peer_send(server, 0, packet) < 0) {
        E_ERROR_RETURN(ErrorType::PeerSendFailed, "Failed to send connection request.", 0);
    }
    return ErrorType::Success;
}

ErrorType NetClient::SendMsg(NetMessage &message)
{
    if (!server || server->state != ENET_PEER_STATE_CONNECTED) {
        E_WARN("Not connected to server. NetMessage not sent.", 0);
        return ErrorType::NotConnected;
    }

    message.connectionToken = connectionToken;
    memset(rawPacket, 0, sizeof(rawPacket));
    size_t bytes = message.Serialize(CSTR0(rawPacket));
    //E_INFO("[SEND][%21s][%5u b] %16s ", rawPacket.dataLength, netMsg.TypeString());
    E_ERROR_RETURN(SendRaw(rawPacket, bytes), "Failed to send packet", 0);
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

ErrorType NetClient::SendSlotClick(SlotId slot, bool doubleClicked)
{
    assert(slot >= 0);
    assert(slot < PlayerInventory::SlotId_Count);

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::SlotClick;
    tempMsg.data.slotClick.slotId = slot;
    tempMsg.data.slotClick.doubleClick = doubleClicked;
    ErrorType result = SendMsg(tempMsg);
    return result;
}

ErrorType NetClient::SendSlotScroll(SlotId slot, int scrollY)
{
    assert(slot >= 0);
    assert(slot < PlayerInventory::SlotId_Count);
    assert(scrollY);

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::SlotScroll;
    tempMsg.data.slotScroll.slotId = slot;
    tempMsg.data.slotScroll.scrollY = scrollY;
    ErrorType result = SendMsg(tempMsg);
    return result;
}


ErrorType NetClient::SendSlotDrop(SlotId slot, uint32_t count)
{
    assert(slot >= 0);
    assert(slot < PlayerInventory::SlotId_Count);
    assert(count);

    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::SlotDrop;
    tempMsg.data.slotDrop.slotId = slot;
    tempMsg.data.slotDrop.count = count;
    ErrorType result = SendMsg(tempMsg);
    return result;
}

ErrorType NetClient::SendTileInteract(float worldX, float worldY)
{
    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.type = NetMessage::Type::TileInteract;
    tempMsg.data.tileInteract.tileX = worldX;
    tempMsg.data.tileInteract.tileY = worldY;
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
    lastInputSentAt = g_clock.now;
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
        //E_WARN("Can't reconcile player; no world");
        return;
    }
    Player *player = serverWorld->FindPlayer(serverWorld->playerId);
    assert(player);
    if (!player) {
        // playerId is invalid??
        E_WARN("Can't reconcile player; no player found", 0);
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
        E_WARN("Can't reconcile player; no snapshot", 0);
        return;
    }

    const Vector3 localPos = player->body.WorldPosition();
    //const Vector3 serverPos = playerSnapshot->position;
    // Client presumably hasn't moved; skip reconciliation
    //if (v3_distance_sq(localPos, serverPos) < SQUARED(CL_MAX_PLAYER_POS_DESYNC_DIST)) {
    //    return;
    //}

    // Roll back local player to server snapshot location
    if (playerSnapshot->flags & PlayerSnapshot::Flags_Position) {
        player->body.Teleport(playerSnapshot->position);

        if (inputHistory.Count()) {
            const InputSample &oldestInput = inputHistory.At(0);
            if (latestSnapshot.lastInputAck + 1 < oldestInput.seq) {
                E_WARN("inputHistory buffer too small. Server ack'd seq #%u on tick %u, but oldest input we still have is seq #%u",
                    latestSnapshot.lastInputAck, latestSnapshot.tick, oldestInput.seq);
            }
        }

        if (g_cl_client_prediction) {
            // Predict player for each input not yet handled by the server
            for (size_t i = 0; i < inputHistory.Count(); i++) {
                InputSample &input = inputHistory.At(i);
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
                    //E_DEBUG("CLI SQ: %u OS: %f S: %f", input.seq, origInput.dt, input.dt);
                    player->Update(input, serverWorld->map);
                }
            }
        }
    }

    // NOTE(dlb): This isn't really necessary, but prevents a _tiny_ amount of stutter vs. when it's
    // turned off. I think it's fine for now..
    if (g_cl_smooth_reconcile) {
        Vector3 oldPos = localPos;
        Vector3 newPos = player->body.WorldPosition();
        Vector3 delta = v3_sub(newPos, oldPos);
        Vector3 smoothDelta = v3_scale(delta, g_cl_smooth_reconcile_factor);
        Vector3 smoothNewPos = v3_add(oldPos, smoothDelta);
        player->body.Teleport(smoothNewPos);
    }
}

void NetClient::ProcessMsg(ENetPacket &packet)
{
    memset(&tempMsg, 0, sizeof(tempMsg));
    tempMsg.Deserialize(packet.data, packet.dataLength);

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
            E_DEBUG("Received world chunk %hd %hd", worldChunk.chunk.x, worldChunk.chunk.y);
#endif
            Tilemap &map = serverWorld->map;
            auto chunkIter = map.chunksIndex.find(worldChunk.chunk.Hash());
            if (chunkIter != map.chunksIndex.end()) {
#if CL_DEBUG_WORLD_CHUNKS
                E_DEBUG("  Updating existing chunk");
#endif
                size_t idx = chunkIter->second;
                assert(idx < map.chunks.size());
                map.chunks[chunkIter->second] = worldChunk.chunk;
            } else {
#if CL_DEBUG_WORLD_CHUNKS
                E_DEBUG("  Adding new chunk to chunk list");
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
        } case NetMessage::Type::TileUpdate: {
            NetMessage_TileUpdate &tileUpdate = tempMsg.data.tileUpdate;

            Tile *tile = serverWorld->map.TileAtWorld(tileUpdate.worldX, tileUpdate.worldY);
            if (tile) {
                tile->type = tileUpdate.tile.type;
                tile->object.type = tileUpdate.tile.object.type;
                tile->object.flags = tileUpdate.tile.object.flags;
            }

            break;
        } case NetMessage::Type::WorldSnapshot: {
            const WorldSnapshot &netSnapshot = tempMsg.data.worldSnapshot;
            WorldSnapshot &worldSnapshot = worldHistory.Alloc();
            worldSnapshot = netSnapshot;
            worldSnapshot.recvAt = g_clock.now;

            const double rtt = server->roundTripTime / 1000.0;
            worldSnapshot.rtt = rtt;

            const double snapTime = g_clock.now; // - rtt;

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
                    //E_DEBUG("Snapshot: player #%u", playerSnapshot.type);
                }

                const bool posChanged = playerSnapshot.flags & PlayerSnapshot::Flags_Position;
                const bool dirChanged = playerSnapshot.flags & PlayerSnapshot::Flags_Direction;

                if (posChanged || dirChanged) {
                    const Vector3Snapshot *prevState{};
                    if (player->body.positionHistory.Count()) {
                        prevState = &player->body.positionHistory.Last();
                    }

                    Vector3Snapshot &state = player->body.positionHistory.Alloc();
                    state.serverTime = snapTime;

                    if (posChanged) {
                        //E_DEBUG("Snapshot: pos %f %f %f",
                            //playerSnapshot.position.x,
                            //playerSnapshot.position.y,
                            //playerSnapshot.position.z);
                        state.v = playerSnapshot.position;
                    } else {
                        if (prevState) {
                            state.v = prevState->v;
                        } else {
                            E_WARN("Received direction update but previous position is not known. playerId: %u", playerSnapshot.id);
                            state.v = player->body.WorldPosition();
                        }
                    }

                    if (dirChanged) {
                        state.direction = playerSnapshot.direction;
                        //E_DEBUG("Snapshot: dir %d", (char)state.direction);
                    } else {
                        if (prevState) {
                            state.direction = prevState->direction;
                            //E_DEBUG("Snapshot: dir %d (fallback prev)", (char)state.direction);
                        } else {
                            E_WARN("Received position update but previous position is not available.", 0);
                            state.direction = player->sprite.direction;
                        }
                    }
                }

                if (playerSnapshot.flags & PlayerSnapshot::Flags_Speed) {
                    player->body.speed = playerSnapshot.speed;
                }
                // TODO: Pos/dir are history based, but these are instantaneous.. hmm.. is that okay?
                if (playerSnapshot.flags & PlayerSnapshot::Flags_Health) {
                    //E_DEBUG("Snapshot: health %f", playerSnapshot.hitPoints);
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
                    //E_DEBUG("Snapshot: healthMax %f", playerSnapshot.hitPointsMax);
                    player->combat.hitPointsMax = playerSnapshot.hitPointsMax;
                }
                if (playerSnapshot.flags & PlayerSnapshot::Flags_Level) {
                    //E_DEBUG("Snapshot: level %u", enemySnapshot.level);
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

            for (size_t i = 0; i < worldSnapshot.npcCount; i++) {
                const NpcSnapshot &npcSnapshot = worldSnapshot.npcs[i];
                if (npcSnapshot.flags & NpcSnapshot::Flags_Despawn) {
                    serverWorld->RemoveNpc(npcSnapshot.id);
                    continue;
                }

                bool spawned = false;
                NPC *npc = serverWorld->FindNpc(npcSnapshot.id);
                if (!npc) {
                    DLB_ASSERT(npcSnapshot.id);
                    DLB_ASSERT(npcSnapshot.type);
                    E_ERROR(serverWorld->SpawnNpc(npcSnapshot.id, npcSnapshot.type, npcSnapshot.position, &npc), "Failed to spawn replicated npc", 0);
                    if (!npc) {
                        continue;
                    }
                    spawned = true;
                }

                if (npcSnapshot.flags & NpcSnapshot::Flags_Name && npcSnapshot.nameLength) {
                    npc->nameLength = MIN(npcSnapshot.nameLength, ENTITY_NAME_LENGTH_MAX);
                    strncpy(npc->name, npcSnapshot.name, npc->nameLength);
                }

                const bool posChanged = npcSnapshot.flags & NpcSnapshot::Flags_Position;
                const bool dirChanged = npcSnapshot.flags & NpcSnapshot::Flags_Direction;

                if (posChanged || dirChanged) {
                    const Vector3Snapshot *prevState{};
                    if (npc->body.positionHistory.Count()) {
                        prevState = &npc->body.positionHistory.Last();
                    }

                    Vector3Snapshot &state = npc->body.positionHistory.Alloc();
                    state.serverTime = snapTime;

                    if (posChanged) {
                        //E_DEBUG("Snapshot: pos %f %f %f",
                        //    enemySnapshot.position.x,
                        //    enemySnapshot.position.y,
                        //    enemySnapshot.position.z);
                        state.v = npcSnapshot.position;
                    } else {
                        if (prevState) {
                            state.v = prevState->v;
                        } else {
                            E_WARN("Received direction update but prevPosition is not known.", 0);
                            state.v = npc->body.WorldPosition();
                        }
                    }

                    if (dirChanged) {
                        state.direction = npcSnapshot.direction;
                        //E_DEBUG("Snapshot: dir %d", (char)state.direction);
                    } else {
                        if (prevState) {
                            state.direction = prevState->direction;
                            //E_DEBUG("Snapshot: dir %d (fallback prev)", (char)state.direction);
                        } else {
                            E_WARN("Received position update but prevState.direction is not available.", 0);
                            state.direction = npc->sprite.direction;
                        }
                    }
                }

                // TODO: Pos/dir are history based, but these are instantaneous.. hmm.. is that okay?
                if (npcSnapshot.flags & NpcSnapshot::Flags_Scale) {
                    //E_DEBUG("Snapshot: scale %f", (char)enemySnapshot.direction);
                    npc->sprite.scale = npcSnapshot.scale;
                }
                if (npcSnapshot.flags & NpcSnapshot::Flags_Health) {
                    //E_DEBUG("Snapshot: health %f", enemySnapshot.hitPoints);
                    if (!spawned) {
                        if (npc->combat.hitPoints && !npcSnapshot.hitPoints) {
                            // Died
                            npc->combat.diedAt = worldSnapshot.recvAt;
                            ParticleEffectParams gooParams{};
                            gooParams.particleCountMin = 50 * (int)ceilf(CL_NPC_CORPSE_LIFETIME);
                            gooParams.particleCountMax = gooParams.particleCountMin;
                            gooParams.durationMin = CL_NPC_CORPSE_LIFETIME;
                            gooParams.durationMax = gooParams.durationMin;
                            serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Goo, npc->WorldCenter(), gooParams);
                            Catalog::g_sounds.Play(Catalog::SoundID::Squish2, 0.5f + dlb_rand32f_variance(0.1f), true);
                        } else if (npc->combat.hitPoints > npcSnapshot.hitPoints) {
                            // Took damage
                            float dmg = npc->combat.hitPoints - npcSnapshot.hitPoints;

                            ParticleEffectParams gooParams{};
                            gooParams.particleCountMin = 5;
                            gooParams.particleCountMax = gooParams.particleCountMin;
                            gooParams.durationMin = 0.5f;
                            gooParams.durationMax = gooParams.durationMin;
                            serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Goo, npc->WorldCenter(), gooParams);

                            ParticleEffectParams dmgParams{};
                            dmgParams.particleCountMin = (int)MAX(1, floorf(log10f(dmg)));
                            dmgParams.particleCountMax = dmgParams.particleCountMin;
                            dmgParams.durationMin = 3.0f;
                            dmgParams.durationMax = dmgParams.durationMin;
                            ParticleEffect *dmgFx = serverWorld->particleSystem.GenerateEffect(Catalog::ParticleEffectID::Number, npc->WorldCenter(), dmgParams);
                            if (dmgFx) {
                                char *text = (char *)calloc(1, 8);
                                snprintf(text, 16, "%.f", dmg);
                                dmgFx->particleCallbacks[(size_t)ParticleEffect_ParticleEvent::Draw] = {
                                    ParticleDrawText,
                                    text
                                };
                                dmgFx->effectCallbacks[(size_t)ParticleEffect_Event::Dying] = {
                                    ParticleFreeText,
                                    text
                                };
                            }

                            Catalog::g_sounds.Play(Catalog::SoundID::Slime_Stab1, 1.0f + dlb_rand32f_variance(0.4f));
                        }
                    }
                    npc->combat.hitPoints = npcSnapshot.hitPoints;
                }
                if (npcSnapshot.flags & NpcSnapshot::Flags_HealthMax) {
                    //E_DEBUG("Snapshot: healthMax %f", enemySnapshot.hitPointsMax);
                    npc->combat.hitPointsMax = npcSnapshot.hitPointsMax;
                }
                if (npcSnapshot.flags & NpcSnapshot::Flags_Level) {
                    //E_DEBUG("Snapshot: level %u", enemySnapshot.level);
                    npc->combat.level = npcSnapshot.level;
                }
            }

            for (size_t i = 0; i < worldSnapshot.itemCount; i++) {
                const ItemSnapshot &itemSnapshot = worldSnapshot.items[i];
                if (itemSnapshot.flags & ItemSnapshot::Flags_Despawn) {
                    serverWorld->itemSystem.Remove(itemSnapshot.id);
                    continue;
                }

                bool spawned = false;
                WorldItem *item = serverWorld->itemSystem.Find(itemSnapshot.id);
                if (!item) {
#if CL_DEBUG_WORLD_ITEMS
                    E_DEBUG("Trying to spawn item: uid %u, count %u, id %u",
                        itemSnapshot.itemUid,
                        itemSnapshot.stackCount,
                        itemSnapshot.id
                    );
#endif
                    item = serverWorld->itemSystem.SpawnItem(
                        itemSnapshot.position,
                        itemSnapshot.itemUid,
                        itemSnapshot.stackCount,
                        itemSnapshot.id
                    );
                    if (!item) {
                        continue;
                    }
                    spawned = true;
                }

                const bool posChanged = itemSnapshot.flags & ItemSnapshot::Flags_Position;
                if (posChanged) {
                    const Vector3Snapshot *prevState{};
                    if (item->body.positionHistory.Count()) {
                        prevState = &item->body.positionHistory.Last();
                    }

                    Vector3Snapshot &state = item->body.positionHistory.Alloc();
                    state.serverTime = snapTime;

                    if (posChanged) {
                        //E_DEBUG("Snapshot: pos %f %f %f",
                        //    itemSnapshot.position.x,
                        //    itemSnapshot.position.y,
                        //    itemSnapshot.position.z);
                        state.v = itemSnapshot.position;
                    } else {
                        if (prevState) {
                            state.v = prevState->v;
                        } else {
                            E_WARN("Received direction update but prevPosition is not known.", 0);
                            state.v = item->body.WorldPosition();
                        }
                    }
                }

                //if (itemSnapshot.flags & ItemSnapshot::Flags_Position) {
                //    item->body.Teleport(itemSnapshot.position);
                //}
                if (itemSnapshot.flags & ItemSnapshot::Flags_ItemUid) {
                    item->stack.uid = itemSnapshot.itemUid;
                }
                if (itemSnapshot.flags & ItemSnapshot::Flags_StackCount) {
                    item->stack.count = itemSnapshot.stackCount;
                    if (!spawned) {
                        if (!item->stack.count && !item->pickedUpAt) {
                            item->pickedUpAt = worldSnapshot.recvAt;
                            Catalog::g_sounds.Play(Catalog::SoundID::Gold, 1.0f + dlb_rand32f_variance(0.2f), true);
                        }
                    }
                }
            }
            break;
        } case NetMessage::Type::GlobalEvent: {
            const NetMessage_GlobalEvent &globalEvent = tempMsg.data.globalEvent;

            switch (globalEvent.type) {
                case NetMessage_GlobalEvent::Type::PlayerJoin: {
                    const NetMessage_GlobalEvent::PlayerJoin &joinEvent = globalEvent.data.playerJoin;
                    PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(joinEvent.playerId);
                    if (!playerInfo) {
                        serverWorld->AddPlayerInfo(joinEvent.name, joinEvent.nameLength, &playerInfo);
                        playerInfo->id = joinEvent.playerId;
                    }
                    break;
                } case NetMessage_GlobalEvent::Type::PlayerLeave: {
                    const NetMessage_GlobalEvent::PlayerLeave &leaveEvent = globalEvent.data.playerLeave;
                    serverWorld->RemovePlayerInfo(leaveEvent.playerId);
                    break;
                }
            }
            break;
        } case NetMessage::Type::NearbyEvent: {
            const NetMessage_NearbyEvent &nearbyEvent = tempMsg.data.nearbyEvent;

            switch (nearbyEvent.type) {
                case NetMessage_NearbyEvent::Type::PlayerState: {
                    E_WARN("NearbyEvent::PlayerState deprecated", 0);
                    break;
                } case NetMessage_NearbyEvent::Type::EnemyState: {
                    E_WARN("NearbyEvent::EnemyState deprecated", 0);
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

        thread_local static const char *prevState = 0;
        const char *curState = ServerStateString();
        if (curState != prevState) {
            E_INFO("%s", curState);
            prevState = curState;
        }

        if (svc > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    E_INFO("Connected to server on port %hu.", event.peer->address.port);

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
                    //E_INFO("RECV\n  %s said %s", senderStr, packet.rawBytes);
                    break;
                } case ENET_EVENT_TYPE_DISCONNECT: {
                    E_INFO("Disconnected from server %hu.", event.peer->address.port);

                    Disconnect();
                    //serverWorld->chatHistory.PushMessage(CSTR("Sam"), CSTR("Disconnected from server."));
                    break;
                } case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    E_WARN("Connection timed out for server on port %hu.", event.peer->address.port);

                    Disconnect();
                    //serverWorld->chatHistory.PushMessage(CSTR("Sam"), CSTR("Your connection to the server timed out. :("));
                    break;
                } default: {
                    E_WARN("Unhandled ENET_EVENT_TYPE %d", event.type);
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