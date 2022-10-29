#include "world.h"
#include "body.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "controller.h"
#include "direction.h"
#include "helpers.h"
#include "maths.h"
#include "entities/entities.h"
#include "particles.h"
#include "player.h"
#include "spritesheet.h"
#include "tilemap.h"
#include "dlb_rand.h"
#include <cassert>

World::World(void)
{
    rtt_seed = 16;
    //rtt_seed = time(NULL);
    dlb_rand32_seed_r(&rtt_rand, rtt_seed, rtt_seed);
    g_noise.Seed(rtt_seed);
}

World::~World(void)
{
}

const Vector3 World::GetWorldSpawn(void)
{
    Vector3 worldSpawn{ 0, 0, 0 };
    return worldSpawn;
};

ErrorType World::AddPlayerInfo(const char *name, uint32_t nameLength, PlayerInfo **result)
{
    DLB_ASSERT(name);
    DLB_ASSERT(nameLength >= USERNAME_LENGTH_MIN);
    DLB_ASSERT(nameLength <= USERNAME_LENGTH_MAX);
    DLB_ASSERT(result);

    PlayerInfo *existingPlayer = FindPlayerInfoByName(name, nameLength);
    if (existingPlayer) {
        return ErrorType::UserAccountInUse;
    }

    for (int i = 0; i < (int)ARRAY_SIZE(playerInfos); i++) {
        PlayerInfo &playerInfo = playerInfos[i];
        if (!playerInfo.nameLength) {
            // We found a free slot
            playerInfo.id = i + 1;
            playerInfo.nameLength = (uint32_t)MIN(nameLength, USERNAME_LENGTH_MAX);
            memcpy(playerInfo.name, name, playerInfo.nameLength);
            *result = &playerInfo;
            return ErrorType::Success;
        }
    }
    return ErrorType::ServerFull;
}

PlayerInfo *World::FindPlayerInfo(uint32_t playerId)
{
    for (PlayerInfo &info : playerInfos) {
        if (info.id == playerId) {
            return &info;
        }
    }
    return 0;
}

PlayerInfo *World::FindPlayerInfoByName(const char *name, size_t nameLength)
{
    if (!name || !nameLength) {
        return 0;
    }

    for (PlayerInfo &playerInfo : playerInfos) {
        if (nameLength == playerInfo.nameLength && !strncmp(name, playerInfo.name, playerInfo.nameLength)) {
            return &playerInfo;
        }
    }
    return 0;
}


void World::RemovePlayerInfo(uint32_t playerId)
{
    RemovePlayer(playerId);

    PlayerInfo *playerInfo = FindPlayerInfo(playerId);
    if (playerInfo) {
        *playerInfo = {};
    }
}

Player *World::AddPlayer(uint32_t playerId)
{
    Player *existingPlayer = FindPlayer(playerId);
    if (existingPlayer) {
        //E_WARN("This playerId is already in use!");
        //return existingPlayer;
        return 0;
    }

    //const PlayerInfo *playerInfo = FindPlayerInfo(playerId);
    //assert(playerInfo->nameLength);

    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (!player.id) {
            assert(!player.combat.hitPointsMax);
            player.id = playerId;
            player.Init();
            if (g_clock.server) {
                player.body.Teleport(GetWorldSpawn());
            }
            return &player;
        }
    }

    // TODO: Make world->players a RingBuffer if this happens; there must
    // be old/invalid player references hanging around (e.g. if there are 8
    // other players and 1 leaves/joins really fast??)
    TraceLog(LOG_ERROR, "Failed to add player");
    return 0;
}

Player *World::FindPlayer(uint32_t playerId)
{
    if (!playerId) {
        return 0;
    }

    for (Player &player : players) {
        if (player.id == playerId) {
            return &player;
        }
    }
    return 0;
}

Player *World::LocalPlayer(void)
{
    return FindPlayer(playerId);
}

Player *World::FindPlayerByName(const char *name, size_t nameLength)
{
    PlayerInfo *playerInfo = FindPlayerInfoByName(name, nameLength);
    return playerInfo ? FindPlayer(playerInfo->id) : 0;
}

Player *World::FindNearestPlayer(Vector2 worldPos, float maxDist, Vector2 *toPlayer)
{
    for (Player &player : players) {
        if (player.id) {
            Vector2 toPlayerVec = v2_sub(player.body.GroundPosition(), worldPos);
            const float toPlayerDistSq = v2_length_sq(toPlayerVec);
            if (toPlayerDistSq <= SQUARED(maxDist)) {
                if (toPlayer) *toPlayer = toPlayerVec;
                return &player;
            }
        }
    }
    return 0;
}

void World::RemovePlayer(uint32_t id)
{
    Player *player = FindPlayer(id);
    if (!player) {
        TraceLog(LOG_ERROR, "Cannot remove a player that doesn't exist (id: %u).", id);
        return;
    }

    E_DEBUG("Remove player %u", id);
    *player = {};
}

ErrorType World::SpawnSam(NPC **result)
{
    NPC *sam = 0;
    E_ERROR_RETURN(SpawnNpc(0, NPC::Type_Slime, { 0, 0, 0 }, &sam), "Failed to spawn Sam", 0);

    DLB_ASSERT(sam);
    sam->SetName(CSTR("Sam"));
    sam->combat.hitPointsMax = 20.0f; //100000.0f;
    sam->combat.hitPoints = sam->combat.hitPointsMax;
    sam->combat.meleeDamage = -1.0f;
    sam->combat.lootTableId = LootTableID::LT_Sam;
    sam->body.Teleport(v3_add(GetWorldSpawn(), { -200.0f, 0, 0 }));
    sam->sprite.scale = 2.0f;

    if (result) *result = sam;
    return ErrorType::Success;
}

ErrorType World::SpawnNpc(uint32_t id, NPC::Type type, Vector3 worldPos, NPC **result)
{
    DLB_ASSERT(type > 0);
    DLB_ASSERT(type < NPC::Type_Count);

    // Only the server should be spawning new npcs; the client merely replicates them by id
    if (g_clock.server) {
        DLB_ASSERT(id == 0);
    } else {
        DLB_ASSERT(id);
    }

    NpcList npcList = npcs.byType[type];
    if (id) {
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (npc.id == id) {
                // TODO: Is this really an error? Just replace the duplicate with the new state, right?
                // Not sure if/when this would ever happen and not be a bug though...
                E_ERROR_RETURN(ErrorType::AllocFailed_Duplicate, "This npc id is already in use!", 0);
            }
        }
    }

    // Find a suitable place to store the new NPC (try to replace by: free slot -> oldest dead)
    NPC *newNpc = 0;
    NPC *oldestDeadNpc = 0;
    NPC *oldestAliveNpc = 0;
    double oldestDeadTime = 0;

    for (size_t i = 0; i < npcList.length; i++) {
        NPC &npc = npcList.data[i];
        if (!npc.id) {
            DLB_ASSERT(!npc.type);
            DLB_ASSERT(!npc.nameLength);
            DLB_ASSERT(!npc.combat.hitPointsMax);
            newNpc = &npc;
            break;
        }

        // Keep track of second/third best types of slots
        double staleTime = g_clock.now - npc.combat.diedAt;
        if (npc.combat.diedAt && staleTime > oldestDeadTime) {
            oldestDeadNpc = &npc;
            oldestDeadTime = staleTime;
        }
    }

    // Reclaim dead slot
    if (!newNpc && oldestDeadNpc) {
        //E_WARN("Replacing oldest dead npc with new npc", 0);
        newNpc = oldestDeadNpc;
        *newNpc = {};
    }

    // Mob cap is full of living mobs
    if (!newNpc) {
        return ErrorType::AllocFailed_Full;
    }

    DLB_ASSERT(newNpc);
    NPC &npc = *newNpc;

    if (id) {
        npc.id = id;
    } else {
        thread_local static uint32_t nextId = 0;
        nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
        npc.id = nextId;
    }

    switch (type) {
        case NPC::Type_Slime: Slime::Init(npc); break;
        case NPC::Type_Townfolk: {
            npc.type = NPC::Type_Townfolk;
            npc.SetName(CSTR("Townfolk"));
            npc.combat.hitPointsMax = 1;
            npc.combat.hitPoints = npc.combat.hitPointsMax;
            npc.combat.flags |= Combat::Flag_TooBigToFail;
            // TODO: Shop inventory?
            //npc.combat.lootTableId = LootTableID::LT_Slime;
            npc.sprite.scale = 1.0f;
            // TODO: FindClosestPlayer and update direction in Townfolk::Update()
            npc.sprite.direction = Direction::South;

            // TODO: Look this up by npc.type in Draw() instead
            if (!g_clock.server) {
                const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Slime);
                const SpriteDef *spriteDef = spritesheet.FindSprite("blue_slime");
                npc.sprite.spriteDef = spriteDef;
            }
            break;
        }
    }

    npc.body.Teleport({ worldPos.x, worldPos.y, 0 });
    E_DEBUG("Spawning npc [%u] @ %.f, %.f", npc.id, worldPos.x, worldPos.y);

    if (result) *result = &npc;
    return ErrorType::Success;

}

NPC *World::FindNpc(uint32_t id)
{
    if (!id) {
        return 0;
    }

    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (npc.id == id) {
                return &npc;
            }
        }
    }
    return 0;
}

void World::RemoveNpc(uint32_t id)
{
    NPC *enemy = FindNpc(id);
    if (!enemy) {
        //TraceLog(LOG_ERROR, "Cannot remove a enemy that doesn't exist. enemyId: %u", enemyId);
        return;
    }

    E_DEBUG("RemoveNPC [%u]", enemy->id);
    *enemy = {};
}

void World::SV_Simulate(double dt)
{
    SV_SimPlayers(dt);
    SV_SimNpcs(dt);
    SV_SimItems(dt);
}

void World::SV_SimPlayers(double dt)
{
    UNUSED(dt);

    for (Player &player : players) {
        if (!player.id) {
            continue;
        }

        if (player.combat.diedAt) {
            if (g_clock.now - player.combat.diedAt < SV_RESPAWN_TIMER) {
                continue;
            } else {
                player.combat.diedAt = 0;
                player.combat.hitPoints = player.combat.hitPointsMax;
                player.body.Teleport(GetWorldSpawn());
            }
        }

        //switch (player.actionState) {
        //    case Player::ActionState::AttackBegin  : printf("%u !!!!\n", tick); break;
        //    case Player::ActionState::AttackSustain: printf("%u **\n", tick); break;
        //    case Player::ActionState::AttackRecover: printf("%u .\n", tick); break;
        //}
        if (player.actionState == Player::ActionState::AttackBegin) {
            const float playerAttackReach = METERS_TO_PIXELS(1.0f);

            const ItemStack selectedStack = player.GetSelectedStack();
            const Item &selectedItem = g_item_db.Find(selectedStack.uid);
            float playerDamage = selectedItem.FindAffix(ItemAffix_DamageFlat).rollValue();
            float playerKnockback = selectedItem.FindAffix(ItemAffix_KnockbackFlat).rollValue();
            if (selectedItem.type == ItemType_Empty) {
                playerDamage = player.combat.meleeDamage;
            }

            for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
                NpcList npcList = npcs.byType[type];
                for (size_t i = 0; i < npcList.length; i++) {
                    NPC &npc = npcList.data[i];
                    if (!npc.id || npc.combat.diedAt) {
                        continue;
                    }

                    // Apply damage to nearby NPCs
                    Vector3 playerToNpc = v3_sub(npc.body.WorldPosition(), player.body.WorldPosition());
                    if (v3_length_sq(playerToNpc) <= SQUARED(playerAttackReach)) {
                        player.stats.damageDealt += npc.TakeDamage(playerDamage);
                        if (playerKnockback) {
                            Vector3 knockbackDir = v3_normalize(playerToNpc);
                            Vector3 knockbackVec = v3_scale(knockbackDir, METERS_TO_PIXELS(playerKnockback));
                            npc.body.ApplyForce(knockbackVec);
                        }
                        if (!npc.combat.hitPoints) {
                            player.xp += dlb_rand32u_range(npc.combat.xpMin, npc.combat.xpMax);
                            int overflowXp = player.xp - (player.combat.level * 20u);
                            if (overflowXp >= 0) {
                                player.combat.level++;
                                player.xp = (uint32_t)overflowXp;

                                // Spawn a merchant for the player
                                if (player.combat.level == 2) {
                                    Vector3 spawnPos = player.body.WorldPosition();
                                    spawnPos.y -= 40;
                                    spawnPos.z = 0;
                                    NPC *level1Merchant = 0;
                                    SpawnNpc(0, NPC::Type_Townfolk, spawnPos, &level1Merchant);
                                    if (level1Merchant) {
                                        PlayerInfo *playerInfo = FindPlayerInfo(player.id);
                                        DLB_ASSERT(playerInfo);
                                        char nameBuf[USERNAME_LENGTH_MAX + 32]{};
                                        int nameLen = snprintf(CSTR0(nameBuf), "%.*s's Merchant", playerInfo->nameLength, playerInfo->name);
                                        level1Merchant->SetName(nameBuf, nameLen);
                                    }
                                }
                            }
                            player.stats.npcsSlain[npc.type]++;
                        }
                    }
                }
            }

            if (pvp) {
                for (Player &otherPlayer : players) {
                    if (otherPlayer.id == player.id || !otherPlayer.id || otherPlayer.combat.diedAt) {
                        continue;
                    }

                    Vector3 playerToPlayer = v3_sub(otherPlayer.body.WorldPosition(), player.body.WorldPosition());
                    if (v3_length_sq(playerToPlayer) <= SQUARED(playerAttackReach)) {
                        player.stats.damageDealt += otherPlayer.combat.TakeDamage(playerDamage);
                        if (!otherPlayer.combat.hitPoints) {
                            player.xp += otherPlayer.combat.level * 10u;
                            int overflowXp = player.xp - (player.combat.level * 20u);
                            if (overflowXp >= 0) {
                                player.combat.level++;
                                player.xp = (uint32_t)overflowXp;
                            }
                            player.stats.playersSlain++;
                        }
                    }
                }
            }
        }

        // Try to spawn enemies near player
        if (!peaceful && dlb_rand32f() < 0.1f) {
            Vector2 spawnPos{};
            spawnPos.x = dlb_rand32f_variance(1.0f);
            spawnPos.y = dlb_rand32f_variance(1.0f);
            spawnPos = v2_normalize(spawnPos);

            // Scale into correct range for valid spawn ring
            float mult = dlb_rand32f_range(SV_ENEMY_MIN_SPAWN_DIST, SV_ENEMY_DESPAWN_RADIUS);
            spawnPos = v2_scale(spawnPos, mult);

            // Translate to whatever point we want to spawn
            Vector2 playerPos = player.body.GroundPosition();
            spawnPos.x += playerPos.x;
            spawnPos.y += playerPos.y;

            const Tile *spawnTile = map.TileAtWorld(spawnPos.x, spawnPos.y);
            if (spawnTile && spawnTile->IsSpawnable()) {
                Player *anyPlayerTooClose = FindNearestPlayer(spawnPos, SV_ENEMY_MIN_SPAWN_DIST);
                if (!anyPlayerTooClose) {
                    SpawnNpc(0, NPC::Type_Slime, { spawnPos.x, spawnPos.y, 0 }, 0);
                }
            }
        }
    }
}

void World::SV_SimNpcs(double dt)
{
    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.id || npc.despawnedAt) {
                continue;
            }

            DLB_ASSERT(npc.type);
            DLB_ASSERT(npc.combat.hitPointsMax);
            npc.Update(*this, dt);
        }
    }
}

void World::SV_SimItems(double dt)
{
    for (WorldItem &item : itemSystem.worldItems) {
        if (!item.euid || item.despawnedAt || g_clock.now < item.spawnedAt + SV_ITEM_PICKUP_DELAY) {
            continue;
        }

        assert(item.stack.uid);
        assert(item.stack.count);

        Player *closestPlayer = FindNearestPlayer(item.body.GroundPosition(), SV_ITEM_ATTRACT_DIST);
        if (!closestPlayer || !closestPlayer->id ||
            (item.droppedByPlayerId == closestPlayer->id && g_clock.now < item.spawnedAt + SV_ITEM_REPICKUP_DELAY)
        ) {
            continue;
        }

        Vector2 itemToPlayer = v2_sub(closestPlayer->body.GroundPosition(), item.body.GroundPosition());
        const float itemToPlayerDistSq = v2_length_sq(itemToPlayer);
        if (itemToPlayerDistSq < SQUARED(SV_ITEM_PICKUP_DIST)) {
            if (closestPlayer->inventory.PickUp(item.stack)) {
                if (!item.stack.count) {
                    item.despawnedAt = g_clock.now;
#if SV_DEBUG_WORLD_ITEMS
                    E_DEBUG("Sim: Item picked up %u", item.type);
#endif
                } else {
                    // TODO: Send item update message so other players know stack was partially picked up
                    // to update their label.
                }
            }
        } else {
            const Vector2 itemToPlayerDir = v2_normalize(itemToPlayer);
            const float speed = MAX(0, 5.0f / PIXELS_TO_METERS(sqrtf(itemToPlayerDistSq)));
            const Vector2 itemVel = v2_scale(itemToPlayerDir, METERS_TO_PIXELS(speed));
            item.body.velocity.x = itemVel.x;
            item.body.velocity.y = itemVel.y;
            //item.body.velocity.z = MAX(item.body.velocity.z, itemVel.z);
        }
    }

    itemSystem.Update(dt);
}

void World::SV_DespawnDeadEntities(void)
{
#if 0
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (!player.type) {
            continue;
        }

        // NOTE: Player is likely going to respawn very quickly, so this may not be useful
        if (!player.combat.hitPoints && g_clock.now - player.combat.diedAt > SV_PLAYER_CORPSE_LIFETIME) {
            E_DEBUG("Despawn stale player corpse %u", player.type);
            DespawnPlayer(player.type);
        }
    }
#endif

    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        if (type == NPC::Type_Townfolk && !g_clock.server) {
            DLB_ASSERT(1);
        }
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.id) {
                continue;
            }

            // Despawn npc if they've been dead for awhile
            if (npc.despawnedAt) {
                const double sinceDespawn = g_clock.now - npc.despawnedAt;
                if (sinceDespawn > SV_NPC_DESPAWN_LIFETIME) {
                    E_DEBUG("Remove despawned npc %u", npc.id);
                    RemoveNpc(npc.id);
                    continue;
                }
            //} else {
            //    const double sinceDeath = npc.combat.diedAt ? (g_clock.now - npc.combat.diedAt) : 0;
            //    if (sinceDeath > SV_NPC_CORPSE_LIFETIME) {
            //        E_DEBUG("Despawn dead npc %u", npc.id);
            //        npc.despawnedAt = g_clock.now;
            //        continue;
            //    }
            }
        }
    }

    itemSystem.DespawnDeadEntities(1.0 / SNAPSHOT_SEND_RATE);
}

void World::CL_Interpolate(double renderAt)
{
    // TODO: Probably would help to unify entities in some way so there's less duplication here
    for (Player &player : players) {
        if (!player.id || player.id == playerId) {
            continue;
        }
        player.body.CL_Interpolate(renderAt, player.sprite.direction);
    }
    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.type) {
                continue;
            }
            npc.body.CL_Interpolate(renderAt, npc.sprite.direction);

            if (npc.type == NPC::Type_Slime && npc.body.Jumped()) {
                //Catalog::SoundID squish = dlb_rand32i_range(0, 1) ? Catalog::SoundID::Squish1 : Catalog::SoundID::Squish2;
                Catalog::SoundID squish = Catalog::SoundID::Squish1;
                Catalog::g_sounds.Play(squish, 1.0f + dlb_rand32f_variance(0.2f), true);
            }
        }
    }
    for (WorldItem &item : itemSystem.worldItems) {
        if (!item.euid) {
            continue;
        }
        item.body.CL_Interpolate(renderAt, item.sprite.direction);
    }
}

void World::CL_Extrapolate(double dt)
{
    // TODO: Collision detection is in Simulate() for enemies, but Player::Update() for players..
    // this makes no sense. It should probably not be in either place. We need a ResolveCollisions, eh?

    for (Player &player : players) {
        if (!player.id || player.id == playerId) {
            continue;
        }
        // TODO: Use future inputs we've received from the server to predict other players more reliability
        InputSample input{};
        input.dt = (float)dt;
        player.Update(input, map);
    }
    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.id) {
                continue;
            }
            npc.Update(*this, dt);
        }
    }
    for (WorldItem &item : itemSystem.worldItems) {
        if (!item.euid || item.despawnedAt || g_clock.now < item.spawnedAt + SV_ITEM_PICKUP_DELAY) {
            continue;
        }

        DLB_ASSERT(item.stack.uid);
        DLB_ASSERT(item.stack.count);
        item.Update(dt);
    }
}

void World::CL_Animate(double dt)
{
    for (Player &player : players) {
        if (!player.id || player.id == playerId) {
            continue;
        }
        player.combat.Update(dt);
    }
    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.id) {
                continue;
            }
            npc.combat.Update(dt);
        }
    }
}

void World::CL_DespawnStaleEntities(void)
{
    Player *player = FindPlayer(playerId);
    if (!player) {
        TraceLog(LOG_ERROR, "Failed to find player");
        return;
    }

#if 0
    for (const Player &otherPlayer : players) {
        if (!otherPlayer.type || otherPlayer.type == playerId) {
            continue;
        }

        const float distSq = v3_length_sq(v3_sub(player->body.WorldPosition(), otherPlayer.body.WorldPosition()));
        const bool faraway = distSq >= SQUARED(CL_PLAYER_FARAWAY_THRESHOLD);
        if (faraway) {
            E_DEBUG("Despawn far away player %u", otherPlayer.type);
            DespawnPlayer(otherPlayer.type);
        }
    }
#endif

    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.id) {
                continue;
            }

            // Remove npc immediately if requested by server
            if (npc.despawnedAt) {
                E_DEBUG("Remove despawned npc id %u", npc.id);
                RemoveNpc(npc.id);
                continue;
            }

            // Remove npc corpse if it's been dead for awhile
            //if (npc.combat.diedAt) {
            //    const double sinceDeath = npc.combat.diedAt ? (g_clock.now - npc.combat.diedAt) : 0;
            //    if (sinceDeath > CL_NPC_CORPSE_LIFETIME) {
            //        E_DEBUG("Remove dead npc %u", npc.id);
            //        RemoveNpc(npc.id);
            //        continue;
            //    }
            //}

#if 0
            if (npc.body.positionHistory.Count()) {
                auto &lastPos = npc.body.positionHistory.Last();

                const float distSq = v3_length_sq(v3_sub(player->body.WorldPosition(), lastPos.v));
                const bool faraway = distSq >= SQUARED(CL_NPC_FARAWAY_THRESHOLD);
                if (faraway) {
                    E_TRACE("Despawn far away npc id %u", npc.id);
                    RemoveNpc(npc.id);
                    continue;
                }

                // TODO: This hack doesn't work since we're only sending snapshots when a slime moves, and slimes don't
                // move unless they have a target. Need to figure out why slime is just chillin and not updating.
                // HACK: Figure out WHY this is happening instead of just despawning. I'm assuming the server just didn't
                // send me a despawn packet for this slime (e.g. because I'm too far away from it??)
                if (g_clock.now - npc.body.positionHistory.Last().recvAt > CL_NPC_STALE_LIFETIME) {
                    //E_WARN("Detected stale npc %u", npc.id);
                    //RemoveSlime(enemy.id);
                    continue;
                }
            }
#endif
        }
    }

    itemSystem.DespawnDeadEntities(0);
}

void World::EnableCulling(Rectangle cullRect)
{
    drawList.EnableCulling(cullRect);
}

bool World::CullTile(Vector2 tilePos, int zoomMipLevel)
{
    if (!drawList.cullEnabled) {
        return false;
    }

    const float tileWidthMip = (float)(TILE_W * zoomMipLevel);
    const float tileHeightMip = (float)(TILE_W * zoomMipLevel);

    if (tilePos.x + tileWidthMip < drawList.cullRect.x
     || tilePos.y + tileHeightMip < drawList.cullRect.y
     || tilePos.x >= drawList.cullRect.x + drawList.cullRect.width
     || tilePos.y >= drawList.cullRect.y + drawList.cullRect.height) {
        return true;
    }

    return false;
}

size_t World::DrawMap(const Spycam &spycam)
{
    const int zoomMipLevel = spycam.GetZoomMipLevel();
    assert(zoomMipLevel > 0);
    if (zoomMipLevel <= 0) {
        return 0;
    }

    //ImDrawList *bgDrawList = ImGui::GetBackgroundDrawList();

    size_t tilesDrawn = 0;
    const Rectangle &camRect = spycam.GetRect();
    const float cx = camRect.x;
    const float cy = camRect.y;
    const float tilesW = camRect.width / TILE_W;
    const float tilesH = camRect.height / TILE_W;
    for (float y = -1; y < tilesH + 2; y += zoomMipLevel) {
        for (float x = -1; x < tilesW + 2; x += zoomMipLevel) {
            float xx = cx + x * TILE_W;
            float yy = cy + y * TILE_W;
            xx = floorf(xx / TILE_W) * TILE_W;
            yy = floorf(yy / TILE_W) * TILE_W;

            const Tile *tile = map.TileAtWorld(xx, yy);
            if (tile) {
                tileset_draw_tile(map.tilesetId, tile->type, { xx, yy }, WHITE);

                //uint8_t r = tile->base;
                //uint8_t g = tile->baseNoise;
                //const int xxi = (int)xx;
                //const int yyi = (int)yy;
                //DrawRectangle(xxi    , yyi     , SUBTILE_W, SUBTILE_H, { r, 0, 0, 255 });  // Top left
                //DrawRectangle(xxi + 8, yyi     , SUBTILE_W, SUBTILE_H, { 0, g, 0, 255 });  // Top right

                //DrawRectangle(xxi    , yyi + 16, SUBTILE_W, SUBTILE_H, { 0, 0, 0, 255 });  // Bottom left
                //DrawRectangle(xxi + 8, yyi + 16, SUBTILE_W, SUBTILE_H, { 0, 0, 0, 255 });  // Bottom right
            } else {
                tileset_draw_tile(map.tilesetId, TileType_Void, { xx, yy }, WHITE);
            }
#if 0
            const Tile *tile = map.TileAtWorld(xx, yy);
            Tileset &tileset = g_tilesets[(size_t)map.tilesetId];
            Rectangle tileRect = tileset_tile_rect(map.tilesetId, tile ? tile->type : TileType_Void);

            ImVec2 uv0 = {
                tileRect.x / tileset.texture.width,
                tileRect.y / tileset.texture.height,
            };
            ImVec2 uv1 = {
                (tileRect.x + tileRect.width ) / tileset.texture.width,
                (tileRect.y + tileRect.height) / tileset.texture.height,
            };

            xx -= cx;
            yy -= cy;

            bgDrawList->AddImage(
                (ImTextureID)(size_t)tileset.texture.type,
                ImVec2(xx, yy),
                ImVec2(xx + tileRect.width, yy + tileRect.height),
                ImVec2{ uv0.x, uv0.y },
                ImVec2{ uv1.x, uv1.y }
            );
#endif
#if 0
            if (xx > 20 && yy > 20) {
                char buf[16]{};
                char *bufPtr = buf + sizeof(buf) - 1;
                int xxp = xx;
                int rem;
                while (xxp) {
                    rem = xxp % 10;
                    *(--bufPtr) = '0' + rem;
                    xxp /= 10;
                }
                //snprintf(buf, sizeof(buf), "%d,\n%d", (int)xx, (int)yy);
                bgDrawList->AddText({ xx - cx, yy - cy }, IM_COL32_WHITE, bufPtr);
            }
#endif
            tilesDrawn++;
        }
    }

    for (float y = -1; y < tilesH + 2; y += zoomMipLevel) {
        for (float x = -1; x < tilesW + 2; x += zoomMipLevel) {
            float xx = cx + x * TILE_W;
            float yy = cy + y * TILE_W;
            xx = floorf(xx / TILE_W) * TILE_W;
            yy = floorf(yy / TILE_W) * TILE_W;

            const Tile *tile = map.TileAtWorld(xx, yy);
            if (tile && tile->object.type) {
                ObjectType effectiveType = tile->object.EffectiveType();
                tileset_draw_tile(TilesetID::TS_Objects, effectiveType, { xx, yy }, WHITE);
            }
        }
    }

    return tilesDrawn;
}

void World::DrawItems(void)
{
    itemSystem.PushAll(drawList);
}

void World::DrawEntities(void)
{
    for (Player &player : players) {
        if (player.id) {
            drawList.Push(player);
        }
    }

    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (npc.id) {
                drawList.Push(npc);
            }
        }
    }
}

void World::DrawParticles(void)
{
    // Queue particles for drawing
    particleSystem.PushAll(drawList);
}

void World::DrawFlush(void)
{
    drawList.Flush(*this);
}