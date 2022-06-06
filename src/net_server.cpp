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
    TraceLog(LOG_DEBUG, "Killing NetServer");
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
    while ((!server || !server->socket)) {
        E_ASSERT(ErrorType::HostCreateFailed, "Failed to create host. Check if port(s) %hu already in use.", socketPort);
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
    ENetBuffer rawPacket{ PACKET_SIZE_MAX, calloc(PACKET_SIZE_MAX, sizeof(uint8_t)) };
    message.Serialize(*serverWorld, rawPacket);

    //E_INFO("[SEND][%21s][%5u b] %16s ", SafeTextFormatIP(client.peer->address), rawPacket.dataLength, netMsg.TypeString());
    if (message.type != NetMessage::Type::WorldSnapshot) {
        const char *subType = "";
        switch (message.type) {
            case NetMessage::Type::GlobalEvent: subType = message.data.globalEvent.TypeString(); break;
            case NetMessage::Type::NearbyEvent: subType = message.data.nearbyEvent.TypeString(); break;
        }
        TraceLog(LOG_DEBUG, "[NetServer] Send %s %s (%zu b)", message.TypeString(), subType, rawPacket.dataLength);
    }

    E_ASSERT(SendRaw(client, rawPacket.data, rawPacket.dataLength), "Failed to send packet");
    free(rawPacket.data);
    return ErrorType::Success;
}

ErrorType NetServer::BroadcastMsg(NetMessage &message)
{
    ErrorType err_code = ErrorType::Success;

    // Broadcast netMsg to all connected clients
    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        SV_Client &client = clients[i];
        ErrorType result = SendMsg(clients[i], message);
        if (result != ErrorType::Success) {
            err_code = result;
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
            welcome.players[i].nameLength = serverWorld->players[i].nameLength;
            memcpy(welcome.players[i].name, serverWorld->players[i].name, welcome.players[i].nameLength);
            welcome.playerCount++;
        }
        E_ASSERT(SendMsg(client, netMsg), "Failed to send welcome basket");
    }

    Player *player = serverWorld->FindPlayer(client.playerId);
    if (!player) {
        TraceLog(LOG_FATAL, "Failed to find player. playerId: %u", client.playerId);
    }

    E_ASSERT(BroadcastPlayerJoin(*player), "Failed to broadcast player join notification");

    const char *message = SafeTextFormat("%.*s joined the game.", player->nameLength, player->name);
    size_t messageLength = strlen(message);
    NetMessage_ChatMessage chatMsg{};
    chatMsg.source = NetMessage_ChatMessage::Source::Server;
    chatMsg.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
    memcpy(chatMsg.message, message, chatMsg.messageLength);
    E_ASSERT(BroadcastChatMessage(chatMsg), "Failed to broadcast player join chat msg");

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

ErrorType NetServer::BroadcastPlayerJoin(const Player &player)
{
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::GlobalEvent;

    NetMessage_GlobalEvent &globalEvent = netMsg.data.globalEvent;
    globalEvent.type = NetMessage_GlobalEvent::Type::PlayerJoin;

    NetMessage_GlobalEvent::PlayerJoin &playerJoin = netMsg.data.globalEvent.data.playerJoin;
    playerJoin.playerId = player.id;
    playerJoin.nameLength = player.nameLength;
    memcpy(playerJoin.name, player.name, player.nameLength);
    return BroadcastMsg(netMsg);
}

ErrorType NetServer::BroadcastPlayerLeave(const Player &player)
{
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::GlobalEvent;

    NetMessage_GlobalEvent &globalEvent = netMsg.data.globalEvent;
    globalEvent.type = NetMessage_GlobalEvent::Type::PlayerLeave;

    NetMessage_GlobalEvent::PlayerLeave &playerLeave = netMsg.data.globalEvent.data.playerLeave;
    playerLeave.playerId = player.id;
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
    E_ASSERT(SendMsg(client, netMsg), "Failed to send chat msg");
    return ErrorType::Success;
}

ErrorType NetServer::SendWorldChunk(const SV_Client &client, const Chunk &chunk)
{
#if SV_DEBUG_WORLD_CHUNKS
    TraceLog(LOG_DEBUG, "Sending world chunk [%hd, %hd] to player #%u", chunk.x, chunk.y, client.playerId);
#endif
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.type = NetMessage::Type::WorldChunk;
    NetMessage_WorldChunk &worldChunk = netMsg.data.worldChunk;
    worldChunk.chunk = chunk;
    E_ASSERT(SendMsg(client, netMsg), "Failed to send world chunk");
    return ErrorType::Success;
}

void NetServer::SendNearbyChunks(SV_Client &client)
{
    // Send nearby chunks to player if they haven't received them yet
    const Player *player = serverWorld->FindPlayer(client.playerId);
    if (player) {
        Vector2 playerBC = player->body.GroundPosition();
        const int16_t chunkX = serverWorld->map->CalcChunk(playerBC.x);
        const int16_t chunkY = serverWorld->map->CalcChunk(playerBC.y);

        for (int y = chunkY - 2; y <= chunkY + 2; y++) {
            for (int x = chunkX - 2; x <= chunkX + 2; x++) {
                const ChunkHash chunkHash = Chunk::Hash(x, y);
                if (!client.chunkHistory.contains(chunkHash)) {
                    const Chunk &chunk = serverWorld->map->FindOrGenChunk(serverWorld->rtt_seed, x, y);
                    SendWorldChunk(client, chunk);
                    client.chunkHistory.insert(chunk.Hash());
                }
            }
        }
    }
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

    worldSnapshot.lastInputAck = client.lastInputAck;
    worldSnapshot.tick = serverWorld->tick;

    // TODO: Find players/slimes/etc. that are actually near the player this snapshot is being generated for
    worldSnapshot.playerCount = 0;
    for (const Player &otherPlayer : serverWorld->players) {
        if (worldSnapshot.playerCount == ARRAY_SIZE(worldSnapshot.players)) {
            break;
        }
        if (!otherPlayer.id) {
            continue;
        }

        // TODO: Make despawn threshold > spawn threshold to prevent spam on event horizon
        const float distSq = v2_length_sq(v2_sub(player.body.GroundPosition(), otherPlayer.body.GroundPosition()));
        bool nearby = distSq <= SQUARED(SV_PLAYER_NEARBY_THRESHOLD);
        if (!nearby) {
#if SV_DEBUG_WORLD_PLAYERS
            TraceLog(LOG_DEBUG, "Left vicinity of player #%u", otherPlayer.id);
#endif
            client.playerHistory.erase(otherPlayer.id);
            continue;
        }

        auto prevState = client.playerHistory.find(otherPlayer.id);
        PlayerSnapshot::Flags flags = PlayerSnapshot::Flags::None;
        if (client.playerId == otherPlayer.id) {
            // Always send player's entire state to to themselves
            // This could be smarter, but if we don't do it, then ReconcilePlayer() can get
            // desync'd from snapshot frequency and "miss" things like teleport events.
            flags = PlayerSnapshot::Flags::Owner;
        } else if (prevState == client.playerHistory.end()) {
            flags = PlayerSnapshot::Flags::PuppetSpawn;
#if SV_DEBUG_WORLD_PLAYERS
            TraceLog(LOG_DEBUG, "Entered vicinity of player #%u", otherPlayer.id);
#endif
        } else {
            // Send delta updates for puppets that the client already knows about

            // "Despawn" notification already sent
            if (prevState->second.flags & PlayerSnapshot::Flags::Despawn) {
                continue;
            } else if (!otherPlayer.combat.hitPoints) {
                flags |= PlayerSnapshot::Flags::Despawn;
            }

            if (!v3_equal(otherPlayer.body.WorldPosition(), prevState->second.position, POSITION_EPSILON)) {
                flags |= PlayerSnapshot::Flags::Position;
            }
            if (otherPlayer.sprite.direction != prevState->second.direction) {
                flags |= PlayerSnapshot::Flags::Direction;
            }
            if (!otherPlayer.combat.hitPoints || (otherPlayer.combat.hitPoints != prevState->second.hitPoints)) {
                flags |= PlayerSnapshot::Flags::Health;
            }
            if (otherPlayer.combat.hitPointsMax != prevState->second.hitPointsMax) {
                flags |= PlayerSnapshot::Flags::HealthMax;
            }
        }

        if (flags == PlayerSnapshot::Flags::None && otherPlayer.id != player.id) {
            continue;
        }

        // TODO: Let Player class serialize itself by storing a reference in the Snapshot, then
        // having NetMessage::Process call a serialize method and forwarding the BitStream
        // and state flags to it.
        PlayerSnapshot &state = client.playerHistory[otherPlayer.id];
        state.flags = flags;
        state.id = otherPlayer.id;
        state.position = otherPlayer.body.WorldPosition();
        state.direction = otherPlayer.sprite.direction;
        state.hitPoints = otherPlayer.combat.hitPoints;
        state.hitPointsMax = otherPlayer.combat.hitPointsMax;
        state.inventory = otherPlayer.inventory;

        worldSnapshot.players[worldSnapshot.playerCount] = state;
        worldSnapshot.playerCount++;
    }

    worldSnapshot.enemyCount = 0;
    for (const Slime &enemy : serverWorld->slimes) {
        if (worldSnapshot.enemyCount == ARRAY_SIZE(worldSnapshot.enemies)) {
            break;
        }
        if (!enemy.id) {
            continue;
        }

        const float distSq = v3_length_sq(v3_sub(player.body.WorldPosition(), enemy.body.WorldPosition()));
        const bool nearby = distSq <= SQUARED(SV_ENEMY_NEARBY_THRESHOLD);
        if (!nearby) {
            client.enemyHistory.erase(enemy.id);
            continue;
        }

        auto prevState = client.enemyHistory.find(enemy.id);
        EnemySnapshot::Flags flags = EnemySnapshot::Flags::None;
        if (prevState == client.enemyHistory.end()) {
            flags = EnemySnapshot::Flags::All;
#if SV_DEBUG_WORLD_ENEMIES
            TraceLog(LOG_DEBUG, "Entered vicinity of enemy #%u", enemy.id);
#endif
        } else {
            // "Despawn" notification already sent
            if (prevState->second.flags & EnemySnapshot::Flags::Despawn) {
                continue;
            } else if (!enemy.combat.hitPoints) {
                flags |= EnemySnapshot::Flags::Despawn;
            }

            if (!v3_equal(enemy.body.WorldPosition(), prevState->second.position, POSITION_EPSILON)) {
                flags |= EnemySnapshot::Flags::Position;
            }
            if (enemy.sprite.direction != prevState->second.direction) {
                flags |= EnemySnapshot::Flags::Direction;
            }
            if (enemy.sprite.scale != prevState->second.scale) {
                flags |= EnemySnapshot::Flags::Scale;
            }
            if (enemy.combat.hitPoints != prevState->second.hitPoints) {
                flags |= EnemySnapshot::Flags::Health;
            }
            if (enemy.combat.hitPointsMax != prevState->second.hitPointsMax) {
                flags |= EnemySnapshot::Flags::HealthMax;
            }
        }

        if (flags == EnemySnapshot::Flags::None) {
            continue;
        }

        // TODO: Let Enemy serialize itself by storing a reference in the Snapshot, then
        // having NetMessage::Process call a serialize method and forwarding the BitStream
        // and state flags to it.
        EnemySnapshot &state = client.enemyHistory[enemy.id];
        state.flags = flags;
        state.id = enemy.id;
        state.position = enemy.body.WorldPosition();
        state.direction = enemy.sprite.direction;
        state.scale = enemy.sprite.scale;
        state.hitPoints = enemy.combat.hitPoints;
        state.hitPointsMax = enemy.combat.hitPointsMax;

        worldSnapshot.enemies[worldSnapshot.enemyCount] = state;
        worldSnapshot.enemyCount++;
    }

    worldSnapshot.itemCount = 0;
    //for (size_t i = 0; i < serverWorld->itemSystem.itemsCount && worldSnapshot.itemCount < ARRAY_SIZE(worldSnapshot.items); i++) {
    for (const ItemWorld &item : serverWorld->itemSystem.items) {
        if (worldSnapshot.itemCount == ARRAY_SIZE(worldSnapshot.items)) {
            break;
        }
        if (!item.id) {
            continue;
        }

        const float distSq = v3_length_sq(v3_sub(player.body.WorldPosition(), item.body.WorldPosition()));
        const bool nearby = distSq <= SQUARED(SV_ITEM_NEARBY_THRESHOLD);
        if (!nearby) {
            client.itemHistory.erase(item.id);
            continue;
        }

        auto prevState = client.itemHistory.find(item.id);
        ItemSnapshot::Flags flags = ItemSnapshot::Flags::None;
        if (prevState == client.itemHistory.end()) {
            flags = ItemSnapshot::Flags::All;
#if SV_DEBUG_WORLD_ITEMS
            TraceLog(LOG_DEBUG, "Entered vicinity of item #%u", item.id);
#endif
        } else {
            // "Despawn" notification already sent
            if (prevState->second.flags & ItemSnapshot::Flags::Despawn) {
                continue;
            } else if (item.pickedUpAt) {
                flags |= ItemSnapshot::Flags::Despawn;
            }

            if (!v3_equal(item.body.WorldPosition(), prevState->second.position, POSITION_EPSILON)) {
                flags |= ItemSnapshot::Flags::Position;
            }
            if (item.stack.id != prevState->second.catalogId) {
                flags |= ItemSnapshot::Flags::CatalogId;
            }
            if (item.stack.count != prevState->second.stackCount) {
                flags |= ItemSnapshot::Flags::StackCount;
            }
        }

        if (flags == ItemSnapshot::Flags::None) {
            continue;
        }

        // TODO: Let Item serialize itself by storing a reference in the Snapshot, then
        // having NetMessage::Process call a serialize method and forwarding the BitStream
        // and state flags to it.
        ItemSnapshot &state = client.itemHistory[item.id];
        state.flags = flags;
        state.id = item.id;
        state.position = item.body.WorldPosition();
        state.catalogId = item.stack.id;
        state.stackCount = item.stack.count;

        // DEBUG(cleanup): FPSDfasdf
        const Vector3 worldPos = item.body.WorldPosition();
#if SV_DEBUG_WORLD_ITEMS
        TraceLog(LOG_DEBUG, "Sending snapshot for Item #%u: itemId: %u count: %u pos: %f %f %f", item.id, item.stack.id, item.stack.count, worldPos.x, worldPos.y, worldPos.z);
#endif
        worldSnapshot.items[worldSnapshot.itemCount] = state;
        worldSnapshot.itemCount++;
    }

    E_ASSERT(SendMsg(client, netMsg), "Failed to send world snapshot");
    client.lastSnapshotSentAt = glfwGetTime();
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

    for (size_t i = 0; i < SV_MAX_SLIMES; i++) {
        Slime &enemy = serverWorld->slimes[i];
        if (!enemy.id) {
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
            TraceLog(LOG_DEBUG, "Entered vicinity of enemy #%u", enemy.id);
        }

        if (!nearby && wasNearby) {
            TraceLog(LOG_DEBUG, "Left vicinity of enemy #%u", enemy.id);
        }

        E_ASSERT(SendEnemyState(client, enemy, nearby, nearby && !wasNearby), "Failed to send nearby enemy state in welcome basket");
    }

#if 0
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &otherPlayer = serverWorld->players[i];
        if (!otherPlayer.id) {
            continue;
        }

        const float distSq     = v3_length_sq(v3_sub(player->body.position, otherPlayer.body.position));
        const float prevDistSq = v3_length_sq(v3_sub(player->body.prevPosition, otherPlayer.body.prevPosition));

        // TODO: Make despawn threshold > spawn threshold to prevent spam on event horizon
        bool nearby    = distSq     <= SQUARED(SV_PLAYER_NEARBY_THRESHOLD);
        bool wasNearby = prevDistSq <= SQUARED(SV_PLAYER_NEARBY_THRESHOLD);
        if (!nearby && !wasNearby) {
            continue;
        }

        bool spawned = nearby && !wasNearby;
        E_ASSERT(SendPlayerState(client, otherPlayer, nearby, spawned), "Failed to send nearby player state in welcome basket");
    }

    for (size_t i = 0; i < SV_MAX_SLIMES; i++) {
        Slime &enemy = serverWorld->slimes[i];
        if (!enemy.id) {
            continue;
        }

        const float distSq     = v3_length_sq(v3_sub(player->body.position, enemy.body.position));
        const float prevDistSq = v3_length_sq(v3_sub(player->body.prevPosition, enemy.body.prevPosition));

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
    state.id = otherPlayer.id;
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
    state.id = enemy.id;
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

ErrorType NetServer::SendItemState(const SV_Client &client, const ItemWorld &item, bool nearby, bool spawned)
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
#define COMMAND_TELEPORT "teleport"

#define SUMMARY_GIVE     "[give] Gives an item to the specified player."
#define SUMMARY_HELP     "[help] Shows the help page for a command."
#define SUMMARY_TELEPORT "[teleport] Teleports a player to a location."

#define USAGE_GIVE       "/give <player> <item_id> <count>"
#define USAGE_HELP       "/help <command>"
#define USAGE_TELEPORT   "/teleport [player] <x> <y> <z>"

    if (!strcmp(command, COMMAND_GIVE)) {
        if (argc == 3) {

        } else {
            SendChatMessage(client, CSTR("[give] Invalid arguments."));
            SendChatMessage(client, CSTR(USAGE_GIVE));
        }
    } else if (!strcmp(command, COMMAND_HELP)) {
        if (argc == 1) {
            if (!strcmp(argv[0], COMMAND_GIVE)) {
                SendChatMessage(client, CSTR(SUMMARY_GIVE));
                SendChatMessage(client, CSTR(USAGE_GIVE));
            } else if (!strcmp(argv[0], COMMAND_HELP)) {
                SendChatMessage(client, CSTR(SUMMARY_HELP));
                SendChatMessage(client, CSTR(USAGE_HELP));
            } else if (!strcmp(argv[0], COMMAND_TELEPORT)) {
                SendChatMessage(client, CSTR(SUMMARY_TELEPORT));
                SendChatMessage(client, CSTR(USAGE_TELEPORT));
            } else {
                const char *err = SafeTextFormat("No help page found for '%s'", argv[0]);
                SendChatMessage(client, err, strlen(err));
            }
        } else {
            SendChatMessage(client, CSTR("[help] Invalid arguments."));
            SendChatMessage(client, CSTR(USAGE_HELP));
        }
    } else if (!strcmp(command, COMMAND_TELEPORT)) {
        if (argc == 3) {
            Player *player = serverWorld->FindPlayer(client.playerId);
            if (player) {
                float x = strtof(argv[0], 0);
                float y = strtof(argv[1], 0);
                float z = strtof(argv[2], 0);
                printf("Teleported %.*s to %f %f %f\n", player->nameLength, player->name, x, y, z);
                player->body.Teleport({ x, y, z });
            }
        } else if (argc == 4) {
            Player *player = serverWorld->FindPlayerByName(argv[0], strlen(argv[0]));
            if (player) {
                float x = strtof(argv[1], 0);
                float y = strtof(argv[2], 0);
                float z = strtof(argv[3], 0);
                printf("Teleported %.*s to %f %f %f\n", player->nameLength, player->name, x, y, z);
                player->body.Teleport({ x, y, z });
            }
        } else {
            SendChatMessage(client, CSTR("[teleport] Invalid arguments."));
            SendChatMessage(client, CSTR(USAGE_TELEPORT));
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

    ENetBuffer packetBuffer{ packet.dataLength, packet.data };
    memset(&netMsg, 0, sizeof(netMsg));
    netMsg.Deserialize(*serverWorld, packetBuffer);

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

            Player *player = serverWorld->AddPlayer(nextPlayerId);
            if (player) {
                player->body.Teleport(serverWorld->GetWorldSpawn());
                nextPlayerId = MAX(1, nextPlayerId + 1); // Prevent ID zero from being used on overflow
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

            // TODO(security): Detect someone sending packets with wrong source/id and PUNISH THEM (.. or wutevs)
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
                        //client.inputBuffer = sample;
                        InputSample &histSample = client.inputHistory.Alloc();
                        histSample = sample;
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
                TraceLog(LOG_DEBUG, "[SRV] SlotDrop  slotId: %u, count: %u", slotDrop.slotId, slotDrop.count);
                ItemStack dropStack = player->inventory.SlotDrop(slotDrop.slotId, slotDrop.count);
                TraceLog(LOG_DEBUG, "[SRV] SpawnItem itemId: %u, count: %u", dropStack.id, dropStack.count);
                serverWorld->itemSystem.SpawnItem(player->body.WorldPosition(), dropStack.id, dropStack.count);
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
        // TODO: Save from identify packet into SV_Client, then user client.username
        Player *player = serverWorld->FindPlayer(client->playerId);
        if (player) {
            E_ASSERT(BroadcastPlayerLeave(*player), "Failed to broadcast player leave notification");

            const char *message = SafeTextFormat("%.*s left the game.", player->nameLength, player->name);
            size_t messageLength = strlen(message);
            NetMessage_ChatMessage chatMsg{};
            chatMsg.source = NetMessage_ChatMessage::Source::Server;
            chatMsg.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
            memcpy(chatMsg.message, message, chatMsg.messageLength);
            E_ASSERT(BroadcastChatMessage(chatMsg), "Failed to broadcast player leave chat msg");
        }

        serverWorld->RemovePlayer(client->playerId);
        *client = {};
    }
    peer->data = 0;
    enet_peer_reset(peer);
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
    while (svc > 0) {
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
    // Notify all clients that the server is stopping
    for (int i = 0; i < server->peerCount; i++) {
        enet_peer_disconnect(&server->peers[i], 0);
    }
    enet_host_service(server, nullptr, 0);
    enet_host_destroy(server);
    assert(sizeof(clients) > 8); // in case i change client list to a pointer and break the memset
    //memset(clients, 0, sizeof(clients));
    *clients = {};
    assert(sizeof(clients) > 8); // in case i change client list to a pointer and break the memset
}