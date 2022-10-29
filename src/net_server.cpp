#include "net_server.h"
#include "bit_stream.h"
#include "helpers.h"
#include "error.h"
#include "users_generated.h"
#include "raylib/raylib.h"
#include "dlb_types.h"

uint8_t NetServer::rawPacket[PACKET_SIZE_MAX];

ErrorType NetServer::SaveUserDB(const char *filename)
{
    flatbuffers::FlatBufferBuilder fbb;

    auto uaGuest = DB::CreateUser(fbb, fbb.CreateString("guest"),   fbb.CreateString("guest"));
    auto uaDandy = DB::CreateUser(fbb, fbb.CreateString("dandy"),   fbb.CreateString("asdf"));
    auto uaOwl   = DB::CreateUser(fbb, fbb.CreateString("owl"),     fbb.CreateString("awesome"));
    auto uaKash  = DB::CreateUser(fbb, fbb.CreateString("kenneth"), fbb.CreateString("shroom"));

    std::vector<flatbuffers::Offset<DB::User>> users{
        uaGuest, uaDandy, uaOwl, uaKash
    };
    auto userDB = DB::CreateUserDBDirect(fbb, &users);

    DB::FinishUserDBBuffer(fbb, userDB);
    if (!SaveFileData(filename, fbb.GetBufferPointer(), fbb.GetSize())) {
        printf("Uh oh, it didn't work :(\n");
    }

    return ErrorType::Success;
}

ErrorType NetServer::LoadUserDB(const char *filename)
{
    fbs_users.data = LoadFileData(filename, &fbs_users.length);
    DLB_ASSERT(fbs_users.length);

    flatbuffers::Verifier verifier(fbs_users.data, fbs_users.length);
    if (!DB::VerifyUserDBBuffer(verifier)) {
        E_ERROR_RETURN(ErrorType::PancakeVerifyFailed, "Uh oh, data verification failed\n", 0);
    }

    const DB::UserDB *userDB = DB::GetUserDB(fbs_users.data);
    const DB::User *dandy = userDB->accounts()->LookupByKey("dandy");
    DLB_ASSERT(dandy);

    return ErrorType::Success;
}

NetServer::NetServer(void)
{
    SaveUserDB("db/users.dat");
    LoadUserDB("db/users.dat");

    //rawPacket.dataLength = PACKET_SIZE_MAX;
    //rawPacket.data = calloc(rawPacket.dataLength, sizeof(uint8_t));
}

NetServer::~NetServer(void)
{
    E_DEBUG("Killing NetServer", 0);
    CloseSocket();
    //free(rawPacket.data);
    UnloadFileData(fbs_users.data);
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
        E_ERROR_RETURN(ErrorType::HostCreateFailed, "Failed to create host. Check if port(s) %hu already in use.", socketPort);
    }

    printf("Listening on port %hu...\n", address.port);
    return ErrorType::Success;
}

ErrorType NetServer::SendRaw(const SV_Client &client, const void *data, size_t size)
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
        E_ERROR_RETURN(ErrorType::PacketCreateFailed, "Failed to create packet.", 0);
    }
    if (enet_peer_send(client.peer, 0, packet) < 0) {
        E_ERROR_RETURN(ErrorType::PeerSendFailed, "Failed to send connection request.", 0);
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
        E_ERROR_RETURN(ErrorType::PacketCreateFailed, "Failed to create packet.", 0);
    }

    // Broadcast netMsg to all connected clients
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        if (!clients[i].peer || clients[i].peer->state != ENET_PEER_STATE_CONNECTED) { // || !clients[i].playerId) {
            continue;
        }

        SV_Client &client = clients[i];
        assert(client.peer);
        assert(client.peer->address.port);
        if (enet_peer_send(client.peer, 0, packet) < 0) {
            TraceLog(LOG_ERROR, "[NetServer] BROADCAST %u bytes failed", size);
            err_code = ErrorType::PeerSendFailed;
        }
    }

    return err_code;
}

ErrorType NetServer::SendMsg(const SV_Client &client, NetMessage &message)
{
    if (!client.peer || client.peer->state != ENET_PEER_STATE_CONNECTED) { // || !client.playerId) {
        return ErrorType::Success;
    }

    message.connectionToken = client.connectionToken;
    memset(rawPacket, 0, sizeof(rawPacket));
    size_t bytes = message.Serialize(CSTR0(rawPacket));

    //E_INFO("[SEND][%21s][%5u b] %16s ", SafeTextFormatIP(client.peer->address), rawPacket.dataLength, netMsg.TypeString());
    if (message.type != NetMessage::Type::WorldSnapshot) {
#if 0
        const char *subType = "";
        switch (message.type) {
            case NetMessage::Type::GlobalEvent: subType = message.data.globalEvent.TypeString(); break;
            case NetMessage::Type::NearbyEvent: subType = message.data.nearbyEvent.TypeString(); break;
        }
        E_DEBUG("[NetServer] Send %s %s (%zu b)", message.TypeString(), subType, bytes);
#endif
    }

    E_ERROR_RETURN(SendRaw(client, rawPacket, bytes), "Failed to send packet", 0);
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastMsg(NetMessage &message, std::function<bool(SV_Client &client)> clientFilter)
{
    ErrorType err_code = ErrorType::Success;

    // Broadcast netMsg to all connected clients
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        SV_Client &client = clients[i];
        if (!clientFilter || clientFilter(client)) {
            ErrorType result = SendMsg(clients[i], message);
            if (result != ErrorType::Success) {
                err_code = result;
            }
        }
    }

    return err_code;
}

ErrorType NetServer::SendWelcomeBasket(SV_Client &client)
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
        welcome.playerId = client.playerId;
        welcome.playerCount = 0;
        for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
            if (!serverWorld->players[i].id)
                continue;

            welcome.players[i].id = serverWorld->players[i].id;
            welcome.players[i].nameLength = serverWorld->playerInfos[i].nameLength;
            memcpy(welcome.players[i].name, serverWorld->playerInfos[i].name, welcome.players[i].nameLength);
            welcome.playerCount++;
        }
        E_ERROR_RETURN(SendMsg(client, netMsg), "Failed to send welcome basket", 0);
    }

    const PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(client.playerId);
    if (!playerInfo) {
        TraceLog(LOG_ERROR, "Failed to find player info. playerId: %u", client.playerId);
    }

    E_ERROR_RETURN(BroadcastPlayerJoin(*playerInfo), "Failed to broadcast player join notification", 0);

    const char *message = SafeTextFormat("%.*s joined the game.", playerInfo->nameLength, playerInfo->name);
    size_t messageLength = strlen(message);
    NetMessage_ChatMessage chatMsg{};
    chatMsg.source = NetMessage_ChatMessage::Source::Server;
    chatMsg.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
    memcpy(chatMsg.message, message, chatMsg.messageLength);
    E_ERROR_RETURN(BroadcastChatMessage(chatMsg), "Failed to broadcast player join chat msg", 0);

    SendNearbyChunks(client);
    SendWorldSnapshot(client);
    //SendNearbyEvents(client);

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

ErrorType NetServer::BroadcastPlayerJoin(const PlayerInfo &playerInfo)
{
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::GlobalEvent;

    NetMessage_GlobalEvent &globalEvent = netMsg.data.globalEvent;
    globalEvent.type = NetMessage_GlobalEvent::Type::PlayerJoin;

    NetMessage_GlobalEvent::PlayerJoin &playerJoin = netMsg.data.globalEvent.data.playerJoin;
    playerJoin.playerId = playerInfo.id;
    playerJoin.nameLength = playerInfo.nameLength;
    memcpy(playerJoin.name, playerInfo.name, playerInfo.nameLength);
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::BroadcastPlayerLeave(uint32_t playerId)
{
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::GlobalEvent;

    NetMessage_GlobalEvent &globalEvent = netMsg.data.globalEvent;
    globalEvent.type = NetMessage_GlobalEvent::Type::PlayerLeave;

    NetMessage_GlobalEvent::PlayerLeave &playerLeave = netMsg.data.globalEvent.data.playerLeave;
    playerLeave.playerId = playerId;
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::SendChatMessage(const SV_Client &client, const char *message, size_t messageLength)
{
    assert(message);
    assert(messageLength <= UINT32_MAX);

    NetMessage_ChatMessage chatMsg{};
    chatMsg.source = NetMessage_ChatMessage::Source::Server;
    chatMsg.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
    memcpy(chatMsg.message, message, chatMsg.messageLength);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::ChatMessage;
    netMsg.data.chatMsg = chatMsg;
    E_ERROR_RETURN(SendMsg(client, netMsg), "Failed to send chat msg", 0);
    return ErrorType::Success;
}

ErrorType NetServer::SendWorldChunk(const SV_Client &client, const Chunk &chunk)
{
#if SV_DEBUG_WORLD_CHUNKS
    E_DEBUG("Sending world chunk [%hd, %hd] to player #%u", chunk.x, chunk.y, client.playerId);
#endif
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::WorldChunk;
    NetMessage_WorldChunk &worldChunk = netMsg.data.worldChunk;
    worldChunk.chunk = chunk;
    E_ERROR_RETURN(SendMsg(client, netMsg), "Failed to send world chunk", 0);
    return ErrorType::Success;
}

void NetServer::SendNearbyChunks(SV_Client &client)
{
    // Send nearby chunks to player if they haven't received them yet
    const Player *player = serverWorld->FindPlayer(client.playerId);
    if (player) {
        Vector2 playerBC = player->body.GroundPosition();
        const int16_t chunkX = serverWorld->map.CalcChunk(playerBC.x);
        const int16_t chunkY = serverWorld->map.CalcChunk(playerBC.y);

        for (int y = chunkY - 2; y <= chunkY + 2; y++) {
            for (int x = chunkX - 2; x <= chunkX + 2; x++) {
                const ChunkHash chunkHash = Chunk::Hash(x, y);
                if (!client.chunkHistory.contains(chunkHash)) {
                    const Chunk &chunk = serverWorld->map.FindOrGenChunk(*serverWorld, x, y);
                    SendWorldChunk(client, chunk);
                    client.chunkHistory.insert(chunk.Hash());
                }
            }
        }
    }
}

ErrorType NetServer::BroadcastTileUpdate(float worldX, float worldY, const Tile &tile)
{
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::TileUpdate;
    NetMessage_TileUpdate &tileUpdate = netMsg.data.tileUpdate;
    tileUpdate.worldX = worldX;
    tileUpdate.worldY = worldY;
    tileUpdate.tile = tile;

    // Broadcast to nearby players
    E_ERROR_RETURN(BroadcastMsg(netMsg, [&](SV_Client &client) {
        const Player *player = serverWorld->FindPlayer(client.playerId);
        if (player) {
            Vector3 playerPos = player->body.WorldPosition();
            if (v3_distance_sq(playerPos, { worldX, worldY, 0 }) < SQUARED(SV_TILE_UPDATE_DIST)) {
                E_INFO("Sending tile update to player %d", client.playerId);
                return true;
            }
        }
        return false;
    }), "Failed to broadcast tile update", 0);

    return ErrorType::Success;
}

ErrorType NetServer::SendWorldSnapshot(SV_Client &client)
{
    assert(client.playerId);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::WorldSnapshot;
    WorldSnapshot &worldSnapshot = netMsg.data.worldSnapshot;

    Player *playerPtr = serverWorld->FindPlayer(client.playerId);
    if (!playerPtr) {
        TraceLog(LOG_ERROR, "Failed to find player to send world snapshot to");
        return ErrorType::PlayerNotFound;
    }
    Player &player = *playerPtr;

    worldSnapshot.tick = serverWorld->tick;
    worldSnapshot.clock = g_clock.now;
    worldSnapshot.lastInputAck = client.lastInputAck;
    worldSnapshot.inputOverflow = client.inputOverflow;

    // TODO: Find players/slimes/etc. that are actually near the player this snapshot is being generated for
    worldSnapshot.playerCount = 0;
    for (const Player &otherPlayer : serverWorld->players) {
        if (!otherPlayer.id) {
            continue;
        }
        if (worldSnapshot.playerCount == ARRAY_SIZE(worldSnapshot.players)) {
            TraceLog(LOG_ERROR, "Snapshot full, skipping player!");
            break;
        }

        uint32_t flags = PlayerSnapshot::Flags_None;
        if (otherPlayer.id == player.id) {
            // Always send player's entire state to to themselves
            // This could be smarter, but if we don't do it, then ReconcilePlayer() can get
            // desync'd from snapshot frequency and "miss" things like teleport events.
            flags = PlayerSnapshot::Flags_Owner;
            if (player.inventory.dirty) {
                flags |= PlayerSnapshot::Flags_Inventory;
                player.inventory.dirty = false;
            }
        } else {
            // TODO: Make despawn threshold > spawn threshold to prevent spam on event horizon
            const float distSq = v2_length_sq(v2_sub(player.body.GroundPosition(), otherPlayer.body.GroundPosition()));
            const bool nearby = distSq <= SQUARED(SV_PLAYER_NEARBY_THRESHOLD);
            const auto prevState = client.playerHistory.find(otherPlayer.id);
            const bool clientAware = prevState != client.playerHistory.end();
            if (nearby) {
                if (clientAware) {
                    // Send delta updates for puppets that the client already knows about
                    if (!v3_equal(otherPlayer.body.WorldPosition(), prevState->second.position, POSITION_EPSILON)) {
                        flags |= PlayerSnapshot::Flags_Position;
                    }
                    if (otherPlayer.sprite.direction != prevState->second.direction) {
                        flags |= PlayerSnapshot::Flags_Direction;
                    }
                    if (!otherPlayer.combat.hitPoints || (otherPlayer.combat.hitPoints != prevState->second.hitPoints)) {
                        flags |= PlayerSnapshot::Flags_Health;
                    }
                    if (otherPlayer.combat.hitPointsMax != prevState->second.hitPointsMax) {
                        flags |= PlayerSnapshot::Flags_HealthMax;
                    }
                    if (otherPlayer.combat.level != prevState->second.level) {
                        flags |= PlayerSnapshot::Flags_Level;
                    }
                } else {
                    flags = PlayerSnapshot::Flags_Spawn;
                    #if SV_DEBUG_WORLD_PLAYERS
                        E_DEBUG("Entered vicinity of player #%u", otherPlayer.id);
                    #endif
                }
            } else {
                if (clientAware) {
                    if (prevState->second.flags & PlayerSnapshot::Flags_Despawn) {
                        // "Despawn" notification already sent, fogetaboutit
                        client.playerHistory.erase(otherPlayer.id);
                        continue;
                    } else {
                        flags |= PlayerSnapshot::Flags_Despawn;
                        #if SV_DEBUG_WORLD_PLAYERS
                            E_DEBUG("Left vicinity of player #%u", otherPlayer.id);
                        #endif
                    }
                }
            }
        }

        if (flags) {
            // TODO: Let Player class serialize itself by storing a reference in the Snapshot, then
            // having NetMessage::Process call a serialize method and forwarding the BitStream
            // and state flags to it.
            PlayerSnapshot &state = client.playerHistory[otherPlayer.id];
            state.flags = flags;
            state.id = otherPlayer.id;
            state.position = otherPlayer.body.WorldPosition();
            state.direction = otherPlayer.sprite.direction;
            state.speed = otherPlayer.body.speed;
            state.hitPoints = otherPlayer.combat.hitPoints;
            state.hitPointsMax = otherPlayer.combat.hitPointsMax;
            state.level = otherPlayer.combat.level;
            state.xp = otherPlayer.xp;
            state.inventory = otherPlayer.inventory;

            worldSnapshot.players[worldSnapshot.playerCount] = state;
            worldSnapshot.playerCount++;
        }
    }

    worldSnapshot.npcCount = 0;
    uint32_t skippedNpcCount = 0;
    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = serverWorld->npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.id) {
                continue;
            }
            DLB_ASSERT(npc.type);
            if (worldSnapshot.npcCount == ARRAY_SIZE(worldSnapshot.npcs)) {
                skippedNpcCount++;
                continue;
            }

            uint32_t flags = NpcSnapshot::Flags_None;
            const float distSq = v3_length_sq(v3_sub(player.body.WorldPosition(), npc.body.WorldPosition()));
            const bool nearby = distSq <= SQUARED(SV_NPC_NEARBY_THRESHOLD);
            const auto prevState = client.npcHistory.find(npc.id);
            const bool clientAware = prevState != client.npcHistory.end();
            if (nearby) {
                if (clientAware) {
                    // Send delta updates for puppets that the client already knows about
                    if (strncmp(prevState->second.name, npc.name, npc.nameLength)) {
                        // TODO: Make NameChangeEvent if it ever actually needs to be updated.. or shared string table
                        flags |= NpcSnapshot::Flags_Name;
                    }
                    if (!v3_equal(npc.body.WorldPosition(), prevState->second.position, POSITION_EPSILON)) {
                        flags |= NpcSnapshot::Flags_Position;
                    }
                    if (npc.sprite.direction != prevState->second.direction) {
                        flags |= NpcSnapshot::Flags_Direction;
                    }
                    if (npc.sprite.scale != prevState->second.scale) {
                        flags |= NpcSnapshot::Flags_Scale;
                    }
                    if (npc.combat.hitPoints != prevState->second.hitPoints) {
                        flags |= NpcSnapshot::Flags_Health;
                    }
                    if (npc.combat.hitPointsMax != prevState->second.hitPointsMax) {
                        flags |= NpcSnapshot::Flags_HealthMax;
                    }
                    if (flags) {
                        #if SV_DEBUG_WORLD_NPCS
                            E_DEBUG("Client aware of npc #%u, flags sent: %s", npc.id, NpcSnapshot::FlagStr(flags));
                        #endif
                    }
                } else if (!npc.despawnedAt) {
                    flags = NpcSnapshot::Flags_Spawn & ~NpcSnapshot::Flags_Name;
                    #if SV_DEBUG_WORLD_NPCS
                        E_DEBUG("Entered vicinity of npc #%u", npc.id);
                    #endif
                }
                flags = NpcSnapshot::Flags_Spawn;
            } else {
                if (clientAware) {
                    if (prevState->second.flags & NpcSnapshot::Flags_Despawn) {
                        // "Despawn" notification already sent, fogetaboutit
                        client.npcHistory.erase(npc.id);
                    } else {
                        flags |= NpcSnapshot::Flags_Despawn;
                        #if SV_DEBUG_WORLD_NPCS
                            E_DEBUG("Left vicinity of npc #%u", npc.uid);
                        #endif
                    }
                }
            }
#if 0
            if (clientAware) {
                if (!nearby || npc.despawnedAt) {
                    // Allow server to despawn enemies close to players (e.g. when the only nearby player
                    // dies), without killing them (which would cause corpses, loot, animations, sfx, etc.)
                    if (prevState->second.flags & NpcSnapshot::Flags_Despawn) {
                        // "Despawn" notification already sent, fogetaboutit
                        client.npcHistory.erase(npc.id);
                        #if SV_DEBUG_WORLD_NPCS
                            E_DEBUG("[dbg_enemy:%u] Already left vicinity of enemy. Erased history.", npc.id);
                        #endif
                    } else {
                        flags |= NpcSnapshot::Flags_Despawn;
                        #if SV_DEBUG_WORLD_NPCS
                            E_DEBUG("[dbg_enemy:%u] Left vicinity of enemy. Sending despawn.", npc.id);
                        #endif
                    }
                } else if (nearby) {
                    // Send delta updates for puppets that the client already knows about
                    if (strncmp(prevState->second.name, npc.name, npc.nameLength)) {
                        // TODO: Make NameChangeEvent if it ever actually needs to be updated.. or shared string table
                        flags |= NpcSnapshot::Flags_Name;
                    }
                    if (true) { //!v3_equal(npc.body.WorldPosition(), prevState->second.position, POSITION_EPSILON)) {
                        flags |= NpcSnapshot::Flags_Position;
                    }
                    if (true) { //npc.sprite.direction != prevState->second.direction) {
                        flags |= NpcSnapshot::Flags_Direction;
                    }
                    if (npc.sprite.scale != prevState->second.scale) {
                        flags |= NpcSnapshot::Flags_Scale;
                    }
                    if (npc.combat.hitPoints != prevState->second.hitPoints) {
                        flags |= NpcSnapshot::Flags_Health;
                    }
                    if (npc.combat.hitPointsMax != prevState->second.hitPointsMax) {
                        flags |= NpcSnapshot::Flags_HealthMax;
                    }
                    if (flags) {
                        #if SV_DEBUG_WORLD_NPCS
                            E_DEBUG("[dbg_enemy:%u] Client aware of enemy and delta sent", npc.id);
                        #endif
                    }
                } else {

                }
            } else if (nearby && !npc.despawnedAt) {
                // Spawn a new puppet
                flags = NpcSnapshot::Flags_OnSpawn;
                #if SV_DEBUG_WORLD_NPCS
                    E_DEBUG("[dbg_enemy:%u] Entered vicinity of enemy. Sending spawn.", npc.id);
                #endif
            }
#endif
            if (flags) {
                // TODO: Let Enemy serialize itself by storing a reference in the Snapshot, then
                // having NetMessage::Process call a serialize method and forwarding the BitStream
                // and state flags to it.
                NpcSnapshot &state = client.npcHistory[npc.id];
                state.flags = flags;
                state.id = npc.id;
                state.type = npc.type;
                state.nameLength = npc.nameLength;
                strncpy(state.name, npc.name, MIN(npc.nameLength, ENTITY_NAME_LENGTH_MAX));
                state.position = npc.body.WorldPosition();
                state.direction = npc.sprite.direction;
                state.scale = npc.sprite.scale;
                state.hitPoints = npc.combat.hitPoints;
                state.hitPointsMax = npc.combat.hitPointsMax;
                state.level = npc.combat.level;

                worldSnapshot.npcs[worldSnapshot.npcCount] = state;
                worldSnapshot.npcCount++;
                //E_DEBUG("SS NPC #%u %s", npc.id, NpcSnapshot::FlagStr(flags));
            }
        }
    }
    if (skippedNpcCount) {
        E_WARN("Snapshot full, skipped %u enemies", skippedNpcCount);
    }

    worldSnapshot.itemCount = 0;
    uint32_t skippedItemCount = 0;
    //for (size_t i = 0; i < serverWorld->itemSystem.itemsCount && worldSnapshot.itemCount < ARRAY_SIZE(worldSnapshot.items); i++) {
    for (const WorldItem &item : serverWorld->itemSystem.worldItems) {
        if (!item.euid) {
            continue;
        }
        if (worldSnapshot.itemCount == ARRAY_SIZE(worldSnapshot.items)) {
            skippedItemCount++;
            continue;
        }

        uint32_t flags = ItemSnapshot::Flags_None;
        const float distSq = v3_length_sq(v3_sub(player.body.WorldPosition(), item.body.WorldPosition()));
        const bool nearby = distSq <= SQUARED(SV_ITEM_NEARBY_THRESHOLD);
        const auto prevState = client.itemHistory.find(item.euid);
        const bool clientAware = prevState != client.itemHistory.end();
        if (nearby) {
            if (clientAware) {
                // Send delta updates for puppets that the client already knows about
                if (!v3_equal(item.body.WorldPosition(), prevState->second.position, POSITION_EPSILON)) {
                    flags |= ItemSnapshot::Flags_Position;
                }
                if (item.stack.uid != prevState->second.itemUid) {
                    flags |= ItemSnapshot::Flags_ItemUid;
                }
                if (item.stack.count != prevState->second.stackCount) {
                    flags |= ItemSnapshot::Flags_StackCount;
                }
            } else if (item.stack.count) {
                // Spawn a new puppet
                flags = ItemSnapshot::Flags_Spawn;
                #if SV_DEBUG_WORLD_ITEMS
                    E_DEBUG("Entered vicinity of item #%u", item.uid);
                #endif
            }
        } else {
            if (clientAware) {
                if (prevState->second.flags & ItemSnapshot::Flags_Despawn) {
                    // "Despawn" notification already sent, fogetaboutit
                    client.itemHistory.erase(item.euid);
                } else {
                    flags |= ItemSnapshot::Flags_Despawn;
                    #if SV_DEBUG_WORLD_ITEMS
                        E_DEBUG("Left vicinity of item #%u", item.uid);
                    #endif
                }
            }
        }

        if (flags) {
            // TODO: Let Item serialize itself by storing a reference in the Snapshot, then
            // having NetMessage::Process call a serialize method and forwarding the BitStream
            // and state flags to it.
            ItemSnapshot &state = client.itemHistory[item.euid];
            state.flags = flags;
            state.id = item.euid;
            state.position = item.body.WorldPosition();
            state.itemUid = item.stack.uid;
            state.stackCount = item.stack.count;

            worldSnapshot.items[worldSnapshot.itemCount] = state;
            worldSnapshot.itemCount++;
        }
    }
    if (skippedItemCount) {
        E_WARN("Snapshot full, skipped %u world items", skippedItemCount);
    }

    E_ERROR_RETURN(SendMsg(client, netMsg), "Failed to send world snapshot", 0);
    client.lastSnapshotSentAt = g_clock.now;
    return ErrorType::Success;
}

#if 0
ErrorType NetServer::SendNearbyEvents(const SV_Client &client)
{
    Player *player = serverWorld->FindPlayer(client.playerId);
    assert(player);
    if (!player) {
        TraceLog(LOG_ERROR, "Failed to find player to send nearby events to");
        return ErrorType::PlayerNotFound;
    }

    for (size_t i = 0; i < SV_MAX_ENEMIES; i++) {
        Slime &enemy = serverWorld->enemies[i];
        if (!enemy.type) {
            continue;
        }

        // TODO: Make despawn threshold > spawn threshold to prevent spam on event horizon
        const float distSq = v2_length_sq(v2_sub(player->body.GroundPosition(), enemy.body.GroundPosition()));
        const float prevDistSq = v2_length_sq(v2_sub(player->body.PrevGroundPosition(), enemy.body.PrevGroundPosition()));
        bool nearby = distSq <= SQUARED(SV_ENEMY_NEARBY_THRESHOLD);
        bool wasNearby = distSq <= SQUARED(SV_ENEMY_NEARBY_THRESHOLD);
        if (!nearby && !wasNearby) {
            continue;
        }

        if (nearby && !wasNearby) {
            E_DEBUG("Entered vicinity of enemy #%u", enemy.type);
        }

        if (!nearby && wasNearby) {
            E_DEBUG("Left vicinity of enemy #%u", enemy.type);
        }

        E_ASSERT(SendEnemyState(client, enemy, nearby, nearby && !wasNearby), "Failed to send nearby enemy state in welcome basket");
    }

#if 0
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &otherPlayer = serverWorld->players[i];
        if (!otherPlayer.type) {
            continue;
        }

        const float distSq     = v3_length_sq(v3_sub(player->body.position, otherPlayer.body.position));
        const float prevDistSq = v3_length_sq(v3_sub(player->body.positionPrev, otherPlayer.body.positionPrev));

        // TODO: Make despawn threshold > spawn threshold to prevent spam on event horizon
        bool nearby    = distSq     <= SQUARED(SV_PLAYER_NEARBY_THRESHOLD);
        bool wasNearby = prevDistSq <= SQUARED(SV_PLAYER_NEARBY_THRESHOLD);
        if (!nearby && !wasNearby) {
            continue;
        }

        bool spawned = nearby && !wasNearby;
        E_ASSERT(SendPlayerState(client, otherPlayer, nearby, spawned), "Failed to send nearby player state in welcome basket");
    }

    for (size_t i = 0; i < SV_MAX_ENEMIES; i++) {
        Slime &enemy = serverWorld->enemies[i];
        if (!enemy.type) {
            continue;
        }

        const float distSq     = v3_length_sq(v3_sub(player->body.position, enemy.body.position));
        const float prevDistSq = v3_length_sq(v3_sub(player->body.positionPrev, enemy.body.positionPrev));

        // TODO: Make despawn threshold > spawn threshold to prevent spam on event horizon
        bool nearby    = distSq     <= SQUARED(SV_ENEMY_NEARBY_THRESHOLD);
        bool wasNearby = prevDistSq <= SQUARED(SV_ENEMY_NEARBY_THRESHOLD);
        if (!nearby && !wasNearby) {
            continue;
        }

        bool spawned = nearby && !wasNearby;
        E_ASSERT(SendEnemyState(client, enemy, nearby, spawned), "Failed to send nearby enemy state in welcome basket");
    }
#endif

    return ErrorType::Success;
}

ErrorType NetServer::SendPlayerState(const SV_Client &client, const Player &otherPlayer, bool nearby, bool spawned)
{
    NetMessage netMsg{};
    netMsg.type = NetMessage::Type::NearbyEvent;

    NetMessage_NearbyEvent &nearbyEvent = netMsg.data.nearbyEvent;
    nearbyEvent.type = NetMessage_NearbyEvent::Type::PlayerState;

    NetMessage_NearbyEvent::PlayerState &state = netMsg.data.nearbyEvent.data.playerState;
    state.type = otherPlayer.type;
    state.nearby = nearby;
    state.spawned = spawned;
    state.attacked = otherPlayer.actionState == Player::ActionState::Attacking;
    state.moved = otherPlayer.moveState != Player::MoveState::Idle;
    // TODO: Proper animation state or hit recovery state for this? Can attacked and tookDamage both be true?
    state.tookDamage = true;
    // TODO: Is this also a state on player.. or? How do we know it happened?
    state.healed = false;
    if (state.spawned || state.moved) {
        state.position = otherPlayer.body.WorldPosition();
        state.direction = otherPlayer.sprite.direction;
    }
    if (state.spawned || state.tookDamage || state.healed) {
        state.hitPoints = otherPlayer.combat.hitPoints;
        state.hitPointsMax = otherPlayer.combat.hitPointsMax;
    }
    return SendMsg(client, netMsg);
}

ErrorType NetServer::SendEnemyState(const SV_Client &client, const Slime &enemy, bool nearby, bool spawned)
{
    NetMessage netMsg{};
    netMsg.type = NetMessage::Type::NearbyEvent;

    NetMessage_NearbyEvent &nearbyEvent = netMsg.data.nearbyEvent;
    nearbyEvent.type = NetMessage_NearbyEvent::Type::EnemyState;

    NetMessage_NearbyEvent::EnemyState &state = netMsg.data.nearbyEvent.data.enemyState;
    state.type = enemy.type;
    state.nearby = nearby;
    state.spawned = spawned;
    state.attacked = enemy.actionState == Slime::ActionState::Attacking;
    state.moved = enemy.moveState != Slime::MoveState::Idle;
    // TODO: Proper animation state or hit recovery state for this? Can attacked and tookDamage both be true?
    state.tookDamage = true;
    // TODO: Is this also a state on slime.. or? How do we know it happened?
    state.healed = false;
    if (state.moved) {
        state.position = enemy.body.WorldPosition();
        state.direction = enemy.sprite.direction;
    }
    if (state.tookDamage || state.healed) {
        state.hitPoints = enemy.combat.hitPoints;
        state.hitPointsMax = enemy.combat.hitPointsMax;
    }
    return SendMsg(client, netMsg);
}

ErrorType NetServer::SendItemState(const SV_Client &client, const WorldItem &item, bool nearby, bool spawned)
{
    assert(!"Not yet implemented");
    return ErrorType::PeerSendFailed;
}
#endif

SV_Client *NetServer::FindClient(uint32_t playerId)
{
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        if (clients[i].playerId == playerId) {
            return &clients[i];
        }
    }
    return 0;
}

bool NetServer::IsValidInput(const SV_Client &client, const InputSample &sample)
{
    // If ownerId doesn't match, ignore
    if (sample.ownerId != client.playerId) {
        // TODO: Disconnect someone trying to impersonate?
        // Maybe their ID changed due to a recent reconnect? Check connection age.
        return false;
    }

    // We already know about this sample, ignore
    if (sample.seq <= client.lastInputRecv) {
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
    //for (size_t i = 0; i < inputHistory.Count(); i++) {
    //    InputSample &histSample = inputHistory.At(i);
    //    if (histSample.ownerId == sample.ownerId && histSample.seq == sample.seq) {
    //        return false;
    //    }
    //}

    return true;
}

bool NetServer::ParseCommand(SV_Client &client, NetMessage_ChatMessage &chatMsg)
{
    if (!chatMsg.messageLength || chatMsg.message[0] != '/') {
        return false;
    }

    // Parse command name
    char *command = strtok(chatMsg.message, "/ ");
    if (!command) {
        return false;
    }

    // Parse command args
    int argc = 0;
    char *argv[SV_COMMAND_MAX_ARGS]{};
    for (;;) {
        argv[argc] = strtok(0, " ");
        if (!argv[argc]) break;
        ++argc;
    };

#if _DEBUG
    printf("Command: %s\n", command);
    for (int i = 0; i < argc; i++) {
        printf("  args[%d]: %s\n", i, argv[i]);
    }
#endif

#define COMMAND_GIVE     "give"
#define COMMAND_HELP     "help"
#define COMMAND_NICK     "nick"
#define COMMAND_PEACE    "peace"
#define COMMAND_RTP      "rtp"
#define COMMAND_SPEED    "speed"
#define COMMAND_TELEPORT "teleport"

#define SUMMARY_GIVE     "Gives an item to the specified player."
#define SUMMARY_HELP     "Shows the help page for a command."
#define SUMMARY_NICK     "Changes a player's nickname."
#define SUMMARY_PEACE    "Toggles peaceful mode."
#define SUMMARY_RTP      "Teleports a player to a random location in the world."
#define SUMMARY_SPEED    "Changes a player's walk speed."
#define SUMMARY_TELEPORT "Teleports a player to a location."

#define USAGE_GIVE       "/give <player> <item_id> <count>"
#define USAGE_HELP       "/help <command>"
#define USAGE_NICK       "/nick [player] nickname"
#define USAGE_PEACE      "/peace"
#define USAGE_RTP        "/rtp [max_distance]"
#define USAGE_SPEED      "/speed [player] <speed>"
#define USAGE_TELEPORT   "/teleport [player] <x> <y> <z>"

    if (!strcmp(command, COMMAND_GIVE)) {
        if (argc == 3) {
            SendChatMessage(client, CSTR("[give] Not yet implemented."));
        } else {
            SendChatMessage(client, CSTR("Usage: " USAGE_GIVE));
        }
    } else if (!strcmp(command, COMMAND_HELP)) {
        if (argc == 0) {
            SendChatMessage(client, CSTR(USAGE_GIVE));
            SendChatMessage(client, CSTR("  " SUMMARY_GIVE));
            SendChatMessage(client, CSTR(USAGE_HELP));
            SendChatMessage(client, CSTR("  " SUMMARY_HELP));
            SendChatMessage(client, CSTR(USAGE_NICK));
            SendChatMessage(client, CSTR("  " SUMMARY_NICK));
            SendChatMessage(client, CSTR(USAGE_PEACE));
            SendChatMessage(client, CSTR("  " SUMMARY_PEACE));
            SendChatMessage(client, CSTR(USAGE_RTP));
            SendChatMessage(client, CSTR("  " SUMMARY_RTP));
            SendChatMessage(client, CSTR(USAGE_SPEED));
            SendChatMessage(client, CSTR("  " SUMMARY_SPEED));
            SendChatMessage(client, CSTR(USAGE_TELEPORT));
            SendChatMessage(client, CSTR("  " SUMMARY_TELEPORT));
        } else if (argc == 1) {
            if (!strcmp(argv[0], COMMAND_GIVE)) {
                SendChatMessage(client, CSTR(USAGE_GIVE));
                SendChatMessage(client, CSTR("  " SUMMARY_GIVE));
            } else if (!strcmp(argv[0], COMMAND_HELP)) {
                SendChatMessage(client, CSTR(USAGE_HELP));
                SendChatMessage(client, CSTR("  " SUMMARY_HELP));
            } else if (!strcmp(argv[0], COMMAND_NICK)) {
                SendChatMessage(client, CSTR(USAGE_NICK));
                SendChatMessage(client, CSTR("  " SUMMARY_NICK));
            } else if (!strcmp(argv[0], COMMAND_PEACE)) {
                SendChatMessage(client, CSTR(USAGE_PEACE));
                SendChatMessage(client, CSTR("  " SUMMARY_PEACE));
            } else if (!strcmp(argv[0], COMMAND_RTP)) {
                SendChatMessage(client, CSTR(USAGE_RTP));
                SendChatMessage(client, CSTR("  " SUMMARY_RTP));
            } else if (!strcmp(argv[0], COMMAND_SPEED)) {
                SendChatMessage(client, CSTR(USAGE_SPEED));
                SendChatMessage(client, CSTR("  " SUMMARY_SPEED));
            } else if (!strcmp(argv[0], COMMAND_TELEPORT)) {
                SendChatMessage(client, CSTR(USAGE_TELEPORT));
                SendChatMessage(client, CSTR("  " SUMMARY_TELEPORT));
            } else {
                const char *err = SafeTextFormat("[help] No help page found for '%s'", argv[0]);
                SendChatMessage(client, err, strlen(err));
            }
        } else {
            SendChatMessage(client, CSTR("Usage: " USAGE_HELP));
        }
    } else if (!strcmp(command, COMMAND_NICK)) {
        if (argc == 1) {
#if 0
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                // TODO: Broadcast name change to other players
                uint32_t nameLength = (uint32_t)strnlen(argv[0], USERNAME_LENGTH_MAX);
                player->nameLength = nameLength;
                strncpy(player->name, argv[0], player->nameLength);
            }
#else
            SendChatMessage(client, CSTR("[nick] Not yet implemented."));
#endif
        } else if (argc == 2) {
            uint32_t nameLength = (uint32_t)strnlen(argv[0], USERNAME_LENGTH_MAX);
            Player *player = serverWorld->FindPlayerByName(argv[0], nameLength);
            if (player) {
#if 0
                // TODO: Broadcast name change to other players
                player->nameLength = nameLength;
                strncpy(player->name, argv[0], player->nameLength);
#else
                SendChatMessage(client, CSTR("[nick] Not yet implemented."));
#endif
            } else {
                SendChatMessage(client, CSTR("[nick] Player not found."));
            }
        } else {
            SendChatMessage(client, CSTR("Usage: " USAGE_NICK));
        }
    } else if (!strcmp(command, COMMAND_PEACE)) {
        // /peace
        if (argc == 0) {
            serverWorld->peaceful = !serverWorld->peaceful;
        } else {
            SendChatMessage(client, CSTR("Usage: " USAGE_PEACE));
        }
    } else if (!strcmp(command, COMMAND_RTP)) {
        if (argc > 1) {
            SendChatMessage(client, CSTR("Usage: " USAGE_RTP));
        } else {
            // /rtp
            float variance = 10000.0f;

            // /rtp [max_distance]
            if (argc == 1) {
                variance = strtof(argv[0], 0);
            }

            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                const PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(player->id);
                assert(playerInfo);

                Vector3 worldPos{};
                worldPos.x += dlb_rand32f_variance(variance);
                worldPos.y += dlb_rand32f_variance(variance);
                player->body.Teleport(worldPos);
                printf("[teleport] Teleported %.*s to %f %f %f\n", playerInfo->nameLength, playerInfo->name, worldPos.x, worldPos.y, worldPos.z);
            }
        }
    } else if (!strcmp(command, COMMAND_SPEED)) {
        // /speed <speed>
        if (argc == 1) {
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                const PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(player->id);
                assert(playerInfo);
                float speed = strtof(argv[0], 0);
                player->body.speed = speed;
                printf("[speed] Set %.*s speed to %f\n", playerInfo->nameLength, playerInfo->name, speed);
            }
        // /speed <player> <speed>
        } else if (argc == 2) {
            Player *player = serverWorld->FindPlayerByName(argv[0], strlen(argv[0]));
            if (player) {
                const PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(player->id);
                assert(playerInfo);
                float speed = strtof(argv[1], 0);
                player->body.speed = speed;
                printf("[speed] Set %.*s speed to %f\n", playerInfo->nameLength, playerInfo->name, speed);
            } else {
                SendChatMessage(client, CSTR("[speed] Player not found."));
            }
        } else {
            SendChatMessage(client, CSTR("Usage: " USAGE_SPEED));
        }
    } else if (!strcmp(command, COMMAND_TELEPORT)) {
        // /teleport <x> <y> <z>
        if (argc == 3) {
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                const PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(player->id);
                assert(playerInfo);
                float x = strtof(argv[0], 0);
                float y = strtof(argv[1], 0);
                float z = strtof(argv[2], 0);
                player->body.Teleport({ x, y, z });
                printf("[teleport] Teleported %.*s to %f %f %f\n", playerInfo->nameLength, playerInfo->name, x, y, z);
            }
        // /teleport <player> <x> <y> <z>
        } else if (argc == 4) {
            Player *player = serverWorld->FindPlayerByName(argv[0], strlen(argv[0]));
            if (player) {
                const PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(player->id);
                assert(playerInfo);
                float x = strtof(argv[1], 0);
                float y = strtof(argv[2], 0);
                float z = strtof(argv[3], 0);
                player->body.Teleport({ x, y, z });
                printf("[teleport] Teleported %.*s to %f %f %f\n", playerInfo->nameLength, playerInfo->name, x, y, z);
            } else {
                SendChatMessage(client, CSTR("[teleport] Player not found."));
            }
        } else {
            SendChatMessage(client, CSTR("Usage: " USAGE_TELEPORT));
        }
    } else {
        const char *err = SafeTextFormat("Unsupported command '%s'", command);
        SendChatMessage(client, err, strlen(err));
    }

    //if (!strncmp(chatMsg.message, "/teleport", chatMsg.messageLength)) {
    //    Player *player = serverWorld->FindPlayer(client.playerId);
    //    if (player) {
    //        Vector2 playerBC = player->body.GroundPosition();
    //        playerBC.x += dlb_rand32f_variance(10000.0f);
    //        playerBC.y += dlb_rand32f_variance(10000.0f);
    //        player->body.Teleport({ playerBC.x, playerBC.y, 0.0f });
    //    }
    //}
    return true;
}

void NetServer::ProcessMsg(SV_Client &client, ENetPacket &packet)
{
    assert(serverWorld);

    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.Deserialize(packet.data, packet.dataLength);

    if (netMsg.type != NetMessage::Type::Identify &&
        netMsg.connectionToken != client.connectionToken)
    {
        // Received a netMsg from a stale connection; discard it
        printf("Ignoring %s packet from stale connection.\n", netMsg.TypeString());
        return;
    }

    //E_INFO("[RECV][%21s][%5u b] %16s ", SafeTextFormatIP(client.peer->address), packet.rawBytes.dataLength, netMsg.TypeString());

    switch (netMsg.type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &identMsg = netMsg.data.identify;

            const DB::UserDB *users = DB::GetUserDB(fbs_users.data);
            const DB::User *user = users->accounts()->LookupByKey(identMsg.username);
            if (user) {
                const char *password = user->password()->data();
                if (!strncmp(identMsg.password, password, PASSWORD_LENGTH_MAX)) {
                    printf("Successful auth as user: %s\n", identMsg.username);
                } else {
                    printf("Incorrect password for user: %s\n", identMsg.username);
                    RemoveClient(client.peer);
                    break;
                }
            } else {
                printf("User does not exist: %s\n", identMsg.username);
                RemoveClient(client.peer);
                break;
            }

            PlayerInfo *playerInfo = 0;
            ErrorType err = serverWorld->AddPlayerInfo(identMsg.username, identMsg.usernameLength, &playerInfo);
            if (err != ErrorType::Success) {
                switch (err) {
                    case ErrorType::UserAccountInUse: {
                        // TODO: Send account already in use netMsg
                        printf("User account already in use: %s\n", identMsg.username);
                        RemoveClient(client.peer);
                        break;
                    }
                    case ErrorType::ServerFull: {
                        // TODO: Send server full netMsg
                        printf("Server full, kicking new user: %s\n", identMsg.username);
                        RemoveClient(client.peer);
                        break;
                    }
                }
                break;
            }

            client.connectionToken = netMsg.connectionToken;
            client.playerId = playerInfo->id;

            assert(identMsg.usernameLength);
            playerInfo->SetName(identMsg.username, identMsg.usernameLength);

            Player *player = serverWorld->AddPlayer(playerInfo->id);
            assert(player);

            // TODO: Load player's spawn location from save file
            player->body.Teleport(serverWorld->GetWorldSpawn());

            // TODO: Load selected slot from save file
            player->inventory.selectedSlot = PlayerInventory::SlotId_Hotbar_0;

            // TODO: Load inventory from save file
            ItemUID longSword = g_item_db.SV_Spawn(ItemType_Weapon_Long_Sword);
            ItemUID dagger = g_item_db.SV_Spawn(ItemType_Weapon_Dagger);
            ItemUID blackBook = g_item_db.SV_Spawn(ItemType_Book_BlackSkull);
            ItemUID silverCoin = g_item_db.SV_Spawn(ItemType_Currency_Silver);
            player->inventory.slots[PlayerInventory::SlotId_Hotbar_0].stack = { longSword, 1 };
            player->inventory.slots[0].stack = { dagger, 1 };
            player->inventory.slots[1].stack = { blackBook, 3 };
            player->inventory.slots[10].stack = { silverCoin, 10 };
            player->inventory.slots[11].stack = { silverCoin, 20 };
            player->inventory.slots[12].stack = { silverCoin, 30 };
            player->inventory.slots[13].stack = { silverCoin, 40 };
            player->inventory.slots[14].stack = { silverCoin, 50 };
            player->inventory.slots[15].stack = { silverCoin, 60 };
            player->inventory.slots[16].stack = { silverCoin, 70 };

            SendWelcomeBasket(client);
            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = netMsg.data.chatMsg;

            // TODO(security): Detect someone sending packets with wrong source/type and PUNISH THEM (.. or wutevs)
            chatMsg.source = NetMessage_ChatMessage::Source::Client;
            chatMsg.id = client.playerId;

            if (!ParseCommand(client, chatMsg)) {
                // Store chat netMsg in chat history
                serverWorld->chatHistory.PushNetMessage(chatMsg);

                // TODO(security): This is currently rebroadcasting user input to all other clients.. ripe for abuse
                // Broadcast chat netMsg
                BroadcastMsg(netMsg);
            }
            break;
        } case NetMessage::Type::Input: {
            NetMessage_Input &input = netMsg.data.input;
            if (input.sampleCount <= CL_INPUT_SAMPLES_MAX) {
                for (size_t i = 0; i < input.sampleCount; i++) {
                    InputSample &sample = input.samples[i];
                    if (sample.seq && IsValidInput(client, sample)) {
                        // NOTE: Will drop oldest input sample when full
                        InputSample &histSample = client.inputHistory.Alloc();
                        histSample = sample;
                        histSample.skipFx = true;
                        client.lastInputRecv = MAX(client.lastInputRecv, sample.seq);
#if SV_DEBUG_INPUT_SAMPLES
                        printf("Received input #%u\n", histSample.seq);
#endif
                    }
                }
            } else {
                // Invalid message format, disconnect naughty user?
            }
            break;
        } case NetMessage::Type::SlotClick: {
            NetMessage_SlotClick &slotClick = netMsg.data.slotClick;
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                // TODO(security): Validate params, discard if invalid
                player->inventory.SlotClick(slotClick.slotId, slotClick.doubleClick);
            }
            break;
        } case NetMessage::Type::SlotScroll: {
            NetMessage_SlotScroll &slotScroll = netMsg.data.slotScroll;
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                // TODO(security): Validate params, discard if invalid
                player->inventory.SlotScroll(slotScroll.slotId, slotScroll.scrollY);
            }
            break;
        } case NetMessage::Type::SlotDrop: {
            NetMessage_SlotDrop &slotDrop = netMsg.data.slotDrop;
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                // TODO(security): Validate params, discard if invalid
                E_DEBUG("[SRV] SlotDrop  slotId: %u, count: %u", slotDrop.slotId, slotDrop.count);
                ItemStack dropStack = player->inventory.SlotDrop(slotDrop.slotId, slotDrop.count);
                if (dropStack.uid && dropStack.count) {
                    E_DEBUG("[SRV] SpawnItem itemUid: %u, count: %u", dropStack.uid, dropStack.count);
                    WorldItem *item = serverWorld->itemSystem.SpawnItem(player->body.WorldPosition(), dropStack.uid, dropStack.count);
                    if (item) {
                        item->droppedByPlayerId = player->id;
                    }
                }
            }
            break;
         } case NetMessage::Type::TileInteract: {
            NetMessage_TileInteract &tileInteract = netMsg.data.tileInteract;

            // TODO(security): Validate params, discard if invalid
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                Tile *tile = serverWorld->map.TileAtWorld(tileInteract.tileX, tileInteract.tileY);
                if (tile && tile->object.IsIteractable()) {
                    switch (tile->object.type) {
                        case ObjectType_Rock01: {
                            // TODO: Move this out to e.g. tile->object.Interact() or something..
                            if (!tile->object.HasFlag(ObjectFlag_Stone_Overturned)) {
                                E_DEBUG("[SRV] TileInteract: Rock attempting to roll loot.", 0);
                                // TODO(v1): Make LootTableID::LT_Rock01
                                // TODO(v2): Look up loot table id based on where the player is
                                // TODO(v3): Account for player's magic find bonus
                                serverWorld->lootSystem.SV_RollDrops(LootTableID::LT_Slime, [&](ItemStack dropStack) {
                                    serverWorld->itemSystem.SpawnItem(
                                        { tileInteract.tileX, tileInteract.tileY, 0 }, dropStack.uid, dropStack.count
                                    );
                                });
                                tile->object.SetFlag(ObjectFlag_Stone_Overturned);
                                BroadcastTileUpdate(tileInteract.tileX, tileInteract.tileY, *tile);
                            } else {
                                E_DEBUG("[SRV] TileInteract: Rock already overturned.", 0);
                            }
                            break;
                        }
                    }
                }
            }
            break;
        } default: {
            E_INFO("Unexpected netMsg type: %s", netMsg.TypeString());
            break;
        }
    }
}

SV_Client *NetServer::AddClient(ENetPeer *peer)
{
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        SV_Client &client = clients[i];
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

SV_Client *NetServer::FindClient(ENetPeer *peer)
{
    SV_Client *client = (SV_Client *)peer->data;
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
    printf("Remove client %s\n", SafeTextFormatIP(peer->address));
    SV_Client *client = FindClient(peer);
    if (client) {
        const PlayerInfo *playerInfo = serverWorld->FindPlayerInfo(client->playerId);
        if (playerInfo && playerInfo->id) {
            E_ERROR_RETURN(BroadcastPlayerLeave(playerInfo->id), "Failed to broadcast player leave notification", 0);

            const char *message = SafeTextFormat("%.*s left the game.", playerInfo->nameLength, playerInfo->name);
            size_t messageLength = strlen(message);
            NetMessage_ChatMessage chatMsg{};
            chatMsg.source = NetMessage_ChatMessage::Source::Server;
            chatMsg.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
            memcpy(chatMsg.message, message, chatMsg.messageLength);
            E_ERROR_RETURN(BroadcastChatMessage(chatMsg), "Failed to broadcast player leave chat msg", 0);

            serverWorld->RemovePlayerInfo(client->playerId);
        }
        *client = {};
    }

    // TODO: Send a packet with a reason back to the client, e.g. for invalid login info, /kick, etc.
    enet_peer_disconnect(peer, 0);
    //peer->data = 0;
    //enet_peer_reset(peer);
    return ErrorType::Success;
}

ErrorType NetServer::Listen(void)
{
    assert(server->address.port);

    // TODO: Do I need to limit the amount of network data processed each "frame" to prevent the simulation from
    // falling behind? How easy is it to overload the server in this manner? Limiting it just seems like it would
    // cause unnecessary latency and bigger problems.. so perhaps just "drop" the remaining packets (i.e. receive
    // the data but don't do anything with it)?

    ENetEvent event{};
    // TODO(dlb): How long should this wait between calls?
    int svc = enet_host_service(server, &event, 1);
    while (1) {
        if (svc < 0) {
            E_ERROR(ErrorType::ENetServiceError, "Unknown network error", 0);
            break;
        } else if (!svc) {
            break;  // No more events
        }
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                E_INFO("A new client connected from %x:%u.", SafeTextFormatIP(event.peer->address));
                AddClient(event.peer);
                break;
            } case ENET_EVENT_TYPE_RECEIVE: {
                //E_INFO("A packet of length %u was received from %x:%u on channel %u.",
                //    event.packet->dataLength,
                //    event.peer->address.host,
                //    event.peer->address.port,
                //    event.channelID);

                SV_Client *client = FindClient(event.peer);
                assert(client);
                if (client) {
                    ProcessMsg(*client, *event.packet);
                }
                enet_packet_destroy(event.packet);
                break;
            } case ENET_EVENT_TYPE_DISCONNECT: {
                E_INFO("%x:%u disconnected.", SafeTextFormatIP(event.peer->address));
                RemoveClient(event.peer);
                break;
            } case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                E_INFO("%x:%u disconnected due to timeout.", SafeTextFormatIP(event.peer->address));
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

void NetServer::CloseSocket(void)
{
    if (!server) return;
    // Notify all clients that the server is stopping
    for (int i = 0; i < (int)server->peerCount; i++) {
        enet_peer_disconnect(&server->peers[i], 0);
    }
    enet_host_service(server, nullptr, 0);
    enet_host_destroy(server);
    assert(sizeof(clients) > 8); // in case i change client list to a pointer and break the memset
    //memset(clients, 0, sizeof(clients));
    *clients = {};
    assert(sizeof(clients) > 8); // in case i change client list to a pointer and break the memset
}