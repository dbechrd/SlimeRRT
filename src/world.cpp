#include "world.h"
#include "body.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "controller.h"
#include "direction.h"
#include "helpers.h"
#include "item_stack.h"
#include "maths.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include "spritesheet.h"
#include "tilemap.h"
#include "dlb_rand.h"
#include <cassert>

World::World(void)
{
    rtt_seed = 16;
    //rtt_seed = time(NULL);
    dlb_rand32_seed_r(&rtt_rand, rtt_seed, rtt_seed);
}

World::~World(void)
{
}

const Vector3 World::GetWorldSpawn(void)
{
    if (!map) {
        return Vector3{};
    }
    Vector3 worldSpawn{ 0, 0, 0 };
    return worldSpawn;
};

Player *World::AddPlayer(uint32_t playerId)
{
    Player *existingPlayer = FindPlayer(playerId);
    if (existingPlayer) {
        //TraceLog(LOG_WARNING, "This playerId is already in use!");
        //return existingPlayer;
        return 0;
    }

    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (player.id == 0) {
            assert(!player.nameLength);
            assert(!player.combat.hitPointsMax);

            if (playerId) {
                player.id = playerId;
            } else {
                nextPlayerId = MAX(1, nextPlayerId + 1); // Prevent ID zero from being used on overflow
                player.id = nextPlayerId;
            }

            const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Charlie);
            const SpriteDef *spriteDef = spritesheet.FindSprite("player_sword");
            player.Init(spriteDef);
            player.body.Teleport(GetWorldSpawn());
            return &player;
        }
    }

    // TODO: Make world->players a RingBuffer if this happens; there must
    // be old/invalid player references hanging around (e.g. if there are 8
    // other players and 1 leaves/joins really fast??)
    TraceLog(LOG_FATAL, "Failed to add player");
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

Player *World::FindPlayerByName(const char *name, size_t nameLength)
{
    if (!name || !nameLength) {
        return 0;
    }

    for (Player &player : players) {
        if (nameLength == player.nameLength && !strncmp(name, player.name, player.nameLength)) {
            return &player;
        }
    }
    return 0;
}

Player *World::FindClosestPlayer(Vector2 worldPos, float maxDist, float *distSq = 0)
{
    for (Player &player : players) {
        if (player.id && player.combat.hitPoints) {
            Vector2 toPlayer = v2_sub(player.body.GroundPosition(), worldPos);
            const float toPlayerDistSq = v2_length_sq(toPlayer);
            if (toPlayerDistSq <= SQUARED(maxDist)) {
                if (distSq) *distSq = toPlayerDistSq;
                return &player;
            }
        }
    }
    return 0;
}

void World::RemovePlayer(uint32_t playerId)
{
    Player *player = FindPlayer(playerId);
    if (!player) {
        TraceLog(LOG_ERROR, "Cannot remove a player that doesn't exist. playerId: %u", playerId);
        return;
    }

    TraceLog(LOG_DEBUG, "Remove player %u", playerId);
    *player = {};
}

Slime &World::SpawnSam(void)
{
    Slime *slime = SpawnSlime(0, {0, 0});
    assert(slime);
    Slime &sam = *slime;
    sam.SetName(CSTR("Sam"));
    sam.combat.hitPointsMax = 20.0f; //100000.0f;
    sam.combat.hitPoints = sam.combat.hitPointsMax;
    sam.combat.meleeDamage = -1.0f;
    sam.combat.lootTableId = LootTableID::LT_Sam;
    sam.body.Teleport(v3_add(GetWorldSpawn(), { -200.0f, 0, 0 }));
    sam.sprite.scale = 2.0f;
    return sam;
}

Slime *World::SpawnSlime(uint32_t slimeId, Vector2 origin)
{
#if _DEBUG
    if (slimeId) {
        for (Slime &slime : slimes) {
            if (slime.id == slimeId) {
                assert(!"This slimeId is already in use!");
            }
        }
    }
#endif

    for (Slime &slime : slimes) {
        if (slime.id == 0) {
            assert(!slime.nameLength);
            assert(!slime.combat.hitPointsMax);

            Vector3 spawnPos{};
            spawnPos.x = dlb_rand32f_variance(1.0f);
            spawnPos.y = dlb_rand32f_variance(1.0f);
            spawnPos = v3_normalize(spawnPos);

            // Scale into correct range for valid spawn ring
            float mult = dlb_rand32f_range(SV_ENEMY_MIN_SPAWN_DIST, SV_ENEMY_DESPAWN_RADIUS);
            spawnPos = v3_scale(spawnPos, mult);

            // Translate to whatever point we want to spawn
            spawnPos.x += origin.x;
            spawnPos.y += origin.y;

            for (Player &player : players) {
                float spawnDist = v3_length_sq(v3_sub(spawnPos, player.body.WorldPosition()));
                if (spawnDist < SV_ENEMY_MIN_SPAWN_DIST * SV_ENEMY_MIN_SPAWN_DIST) {
                    //TraceLog(LOG_DEBUG, "Failed to spawn enemy, too close to player");
                    //return 0;
                }
            }

            if (slimeId) {
                slime.id = slimeId;
            } else {
                thread_local uint32_t nextId = 0;
                nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
                slime.id = nextId;
            }

            TraceLog(LOG_DEBUG, "SpawnSlime [%u]", slime.id);
            slime.Init();
            slime.body.Teleport(spawnPos);
            //TraceLog(LOG_DEBUG, "Spawned enemy %u", slime.type);
            return &slime;
        }
    }

    TraceLog(LOG_ERROR, "Failed to spawn enemy");
    return 0;
}

Slime *World::FindSlime(uint32_t slimeId)
{
    if (!slimeId) {
        return 0;
    }

    for (Slime &slime : slimes) {
        if (slime.id == slimeId) {
            return &slime;
        }
    }
    return 0;
}

void World::RemoveSlime(uint32_t slimeId)
{
    Slime *slime = FindSlime(slimeId);
    if (!slime) {
        TraceLog(LOG_ERROR, "Cannot remove a slime that doesn't exist. slimeId: %u", slimeId);
        return;
    }

    TraceLog(LOG_DEBUG, "RemoveSlime [%u]", slime->id);
    *slime = {};
}

void World::SV_Simulate(double dt)
{
    SV_SimPlayers(dt);
    SV_SimSlimes(dt);
    SV_SimItems(dt);
}

void World::SV_SimPlayers(double dt)
{
    for (Player &player : players) {
        if (!player.id) {
            continue;
        }

        if (player.combat.diedAt) {
            if (glfwGetTime() - player.combat.diedAt < SV_RESPAWN_TIMER) {
                continue;
            } else {
                player.combat.diedAt = 0;
                player.combat.hitPoints = player.combat.hitPointsMax;
                player.body.Teleport(GetWorldSpawn());
            }
        }

        //assert(map);
        //player.Update(dt, *map);

        if (player.actionState == Player::ActionState::Attacking && player.combat.attackFrame == 1) {
            const float playerAttackReach = METERS_TO_PIXELS(1.0f);
            float playerDamage;

            const ItemStack &selectedStack = player.GetSelectedItem();
            const Catalog::Item &selectedItem = Catalog::g_items.Find(selectedStack.itemType);
            playerDamage = selectedItem.damage;
            if (selectedItem.itemType == ITEMTYPE_EMPTY) {
                playerDamage = player.combat.meleeDamage;
            }

            for (Slime &slime : slimes) {
                if (!slime.id || slime.combat.diedAt) {
                    continue;
                }

                Vector3 playerToSlime = v3_sub(slime.body.WorldPosition(), player.body.WorldPosition());
                if (v3_length_sq(playerToSlime) <= SQUARED(playerAttackReach)) {
                    player.stats.damageDealt += slime.combat.DealDamage(playerDamage);
                    if (!slime.combat.hitPoints) {
                        player.xp += dlb_rand32u_range(slime.combat.xpMin, slime.combat.xpMax);
                        int overflowXp = player.xp - (player.combat.level * 20u);
                        if (overflowXp >= 0) {
                            player.combat.level++;
                            player.xp = (uint32_t)overflowXp;
                        }
                        player.stats.slimesSlain++;
                    }
                }
            }

            for (Player &otherPlayer : players) {
                if (otherPlayer.id == player.id || !otherPlayer.id || otherPlayer.combat.diedAt) {
                    continue;
                }

                Vector3 playerToPlayer = v3_sub(otherPlayer.body.WorldPosition(), player.body.WorldPosition());
                if (v3_length_sq(playerToPlayer) <= SQUARED(playerAttackReach)) {
                    player.stats.damageDealt += otherPlayer.combat.DealDamage(playerDamage);
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

        // Try to spawn enemies near player
        if (dlb_rand32f() < 2.0f * dt) {
            SpawnSlime(0, player.body.GroundPosition());
        }
    }
}

void World::SV_SimSlimes(double dt)
{
    for (size_t slimeIdx = 0; slimeIdx < SV_MAX_SLIMES; slimeIdx++) {
        Slime &slime = slimes[slimeIdx];
        if (!slime.id) {
            continue;
        }

        if (slime.combat.diedAt) {
            if (!slime.combat.droppedDeathLoot) {
                uint32_t coins = lootSystem.RollCoins(slime.combat.lootTableId, (int)slime.sprite.scale);
                assert(coins);

                //ItemType coinType{};
                //const float chance = dlb_rand32f_range(0, 1);
                //if (chance < 100.0f / 111.0f) {
                //    coinType = ItemType::Currency_Copper;
                //} else if (chance < 10.0f / 111.0f) {
                //    coinType = ItemType::Currency_Silver;
                //} else {
                //    coinType = ItemType::Currency_Gilded;
                //}

                ItemType itemId = dlb_rand32u_range(2, ITEMTYPE_COUNT - 1);
                uint32_t rndCount = dlb_rand32u_range(1, 4);
                uint32_t itemCount = MIN(rndCount, Catalog::g_items.Find(itemId).stackLimit);
                itemSystem.SpawnItem(slime.WorldCenter(), itemId, itemCount);
                //itemSystem.SpawnItem(slime.WorldCenter(), coinType, coins);
                slime.combat.droppedDeathLoot = true;
            }
            continue;
        }

        // Find nearest player
        float slimeToPlayerDistSq = 0.0f;
        Player *closestPlayer = FindClosestPlayer(slime.body.GroundPosition(), SV_ENEMY_DESPAWN_RADIUS, &slimeToPlayerDistSq);
        if (!closestPlayer) {
            // No nearby players, insta-kill slime w/ no loot
            slime.combat.DealDamage(slime.combat.hitPoints);
            slime.combat.droppedDeathLoot = true;
            continue;
        }

        // Allow slime to move toward nearest player
        if (slimeToPlayerDistSq <= SQUARED(SV_SLIME_ATTACK_TRACK)) {
            Vector2 slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), slime.body.GroundPosition());
            const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
            const float moveDist = MIN(slimeToPlayerDist, METERS_TO_PIXELS(slime.body.speed) * slime.sprite.scale);
            // 5% -1.0, 95% +1.0f
            const float moveRandMult = 1.0f; //dlb_rand32i_range(1, 100) > 5 ? 1.0f : -1.0f;
            const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
            const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
            const Vector3 slimePos = slime.body.WorldPosition();
            const Vector3 slimePosNew = v3_add(slimePos, { slimeMove.x, slimeMove.y, 0 });

            int willCollide = 0;
            for (size_t collideIdx = slimeIdx + 1; collideIdx < SV_MAX_SLIMES; collideIdx++) {
                Slime &otherSlime = slimes[collideIdx];
                if (!otherSlime.id || otherSlime.combat.diedAt) {
                    continue;
                }

                Vector3 otherSlimePos = otherSlime.body.WorldPosition();
                const float radiusScaled = SV_SLIME_RADIUS * slime.sprite.scale;
                if (v3_length_sq(v3_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled)) {
                    slime.TryCombine(otherSlime);
                }
                if (v3_length_sq(v3_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled)) {
                    willCollide = 1;
                }
            }

            if (!willCollide && slime.Move(dt, slimeMove)) {
                // TODO(cleanup): used to play sound effect, might do something else on server?
            }
        }

        // Allow slime to attack if on the ground and close enough to the player
        if (slimeToPlayerDistSq <= SQUARED(SV_SLIME_ATTACK_REACH)) {
            if (slime.Attack(dt)) {
                closestPlayer->combat.DealDamage(slime.combat.meleeDamage * slime.sprite.scale);
            }
        }

        slime.Update(dt);
    }
}

void World::SV_SimItems(double dt)
{
    const double now = glfwGetTime();

    for (ItemWorld &item : itemSystem.items) {
        if (!item.uid || item.pickedUpAt || now < item.spawnedAt + SV_ITEM_PICKUP_DELAY) {
            continue;
        }

        assert(item.stack.itemType != ITEMTYPE_EMPTY);

        Player *closestPlayer = FindClosestPlayer(item.body.GroundPosition(), SV_ITEM_ATTRACT_DIST);
        if (!closestPlayer || !closestPlayer->id ||
            (item.droppedByPlayerId == closestPlayer->id && now < item.spawnedAt + SV_ITEM_REPICKUP_DELAY)
        ) {
            continue;
        }

        Vector2 itemToPlayer = v2_sub(closestPlayer->body.GroundPosition(), item.body.GroundPosition());
        const float itemToPlayerDistSq = v2_length_sq(itemToPlayer);
        if (itemToPlayerDistSq < SQUARED(SV_ITEM_PICKUP_DIST)) {
            if (closestPlayer->inventory.PickUp(item.stack)) {
                if (!item.stack.count) {
                    item.pickedUpAt = now;
#if SV_DEBUG_WORLD_ITEMS
                    TraceLog(LOG_DEBUG, "Sim: Item picked up %u", item.type);
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

void World::DespawnDeadEntities(void)
{
    double now = glfwGetTime();

#if 0
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (!player.type) {
            continue;
        }

        // NOTE: Player is likely going to respawn very quickly, so this may not be useful
        if (!player.combat.hitPoints && glfwGetTime() - player.combat.diedAt > SV_PLAYER_CORPSE_LIFETIME) {
            TraceLog(LOG_DEBUG, "Despawn stale player corpse %u", player.type);
            DespawnPlayer(player.type);
        }
    }
#endif

    for (const Slime &enemy : slimes) {
        if (!enemy.id) {
            continue;
        }

        // Check if enemy has been dead for awhile
        if (enemy.combat.diedAt && now - enemy.combat.diedAt > SV_ENEMY_CORPSE_LIFETIME) {
            //TraceLog(LOG_DEBUG, "Despawn stale enemy corpse %u", enemy.id);
            RemoveSlime(enemy.id);
        }
    }

    itemSystem.DespawnDeadEntities(1.0 / SNAPSHOT_SEND_RATE);
}

bool World::CL_InterpolateBody(Body3D &body, double renderAt, Direction &direction)
{
    auto positionHistory = body.positionHistory;
    const size_t historyLen = positionHistory.Count();

    // If no history, nothing to interpolate yet
    if (historyLen == 0) {
        //printf("No snapshot history to interpolate from\n");
        return true;
    }

    // Find first snapshot after renderAt time
    size_t right = 0;
    while (right < historyLen && positionHistory.At(right).recvAt <= renderAt) {
        right++;
    }

    // renderAt is before any snapshots, show entity at oldest snapshot
    if (right == 0) {
        Vector3Snapshot &oldest = positionHistory.At(0);
        assert(renderAt < oldest.recvAt);
        //printf("renderAt %f before oldest snapshot %f\n", renderAt, oldest.recvAt);

        body.Teleport(oldest.v);
        direction = oldest.direction;
    // renderAt is after all snapshots, show entity at newest snapshot
    } else if (right == historyLen) {
        // TODO: Extrapolate beyond latest snapshot if/when this happens? Should be mostly avoidable..
        Vector3Snapshot &newest = positionHistory.At(historyLen - 1);
        assert(renderAt >= newest.recvAt);
        if (renderAt > newest.recvAt) {
            //printf("renderAt %f after newest snapshot %f\n", renderAt, newest.recvAt);
        }

        // TODO: Send explicitly despawn event from server
        // If we haven't seen an entity in 2 snapshots, chances are it's gone
        if (renderAt > newest.recvAt + (1.0 / SNAPSHOT_SEND_RATE) * 2) {
            //printf("Despawning body due to inactivity\n");
            return false;
        }

        body.Teleport(newest.v);
        direction = newest.direction;
        // renderAt is between two snapshots
    } else {
        assert(right > 0);
        assert(right < historyLen);
        Vector3Snapshot &a = positionHistory.At(right - 1);
        Vector3Snapshot &b = positionHistory.At(right);

        if (renderAt < a.recvAt) {
            assert(renderAt);
        }
        assert(renderAt >= a.recvAt);
        assert(renderAt < b.recvAt);

        // Linear interpolation: body.x = x0 + (x1 - x0) * alpha;
        double alpha = (renderAt - a.recvAt) / (b.recvAt - a.recvAt);
        const Vector3 interpPos = v3_add(a.v, v3_scale(v3_sub(b.v, a.v), (float)alpha));
        body.Teleport(interpPos);
        direction = b.direction;
    }

    return true;
}

void World::CL_Interpolate(double renderAt)
{
    // TODO: Probably would help to unify entities in some way so there's less duplication here
    for (Player &player : players) {
        if (!player.id || player.id == playerId) {
            continue;
        }
        CL_InterpolateBody(player.body, renderAt, player.sprite.direction);
    }
    for (Slime &enemy : slimes) {
        if (!enemy.id) {
            continue;
        }
        CL_InterpolateBody(enemy.body, renderAt, enemy.sprite.direction);
    }
    for (ItemWorld &item : itemSystem.items) {
        if (!item.uid) {
            continue;
        }
        CL_InterpolateBody(item.body, renderAt, item.sprite.direction);
    }
}

void World::CL_Extrapolate(double dt)
{
    // If there's no map, we can't do collision detection
    if (!map) return;

    // TODO: Collision detection is in Simulate() for enemies, but Player::Update() for players..
    // this makes no sense. It should probably not be in either place. We need a ResolveCollisions, eh?

    for (Player &player : players) {
        if (!player.id || player.id == playerId) {
            continue;
        }
        // TODO: Use future inputs we've received from the server to predict other players more reliability
        InputSample input{};
        double dtMs = dt * 1000.0;
        if (dtMs > UINT8_MAX) {
            TraceLog(LOG_WARNING, "Extrapolation dt too large, will be truncated to 256 ms");
        }
        input.msec = (uint8_t)MIN(dt * 1000.0, UINT8_MAX);
        player.Update(input, *map);
    }
    for (Slime &enemy : slimes) {
        if (!enemy.id) {
            continue;
        }
        enemy.Update(dt);
    }
    for (ItemWorld &item : itemSystem.items) {
        if (!item.uid) {
            continue;
        }
        item.Update(dt);
    }
}

void World::CL_DespawnStaleEntities(void)
{
    DespawnDeadEntities();

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
            TraceLog(LOG_DEBUG, "Despawn far away player %u", otherPlayer.type);
            DespawnPlayer(otherPlayer.type);
        }
    }
#endif

    for (const Slime &enemy : slimes) {
        if (!enemy.id) {
            continue;
        }

        if (enemy.body.positionHistory.Count()) {
            auto &lastPos = enemy.body.positionHistory.Last();

            const float distSq = v3_length_sq(v3_sub(player->body.WorldPosition(), lastPos.v));
            const bool faraway = distSq >= SQUARED(CL_ENEMY_FARAWAY_THRESHOLD);
            if (faraway) {
                //TraceLog(LOG_DEBUG, "Despawn far away enemy %u", enemy.type);
                RemoveSlime(enemy.id);
                continue;
            }

            // TODO: This hack doesn't work since we're only sending snapshots when a slime moves, and slimes don't
            // move unless they have a target. Need to figure out why slime is just chillin and not updating.
            // HACK: Figure out WHY this is happening instead of just despawning. I'm assuming the server just didn't
            // send me a despawn packet for this slime (e.g. because I'm too far away from it??)
            //if (glfwGetTime() - enemy.body.positionHistory.Last().recvAt > SV_ENEMY_STALE_LIFETIME) {
            //    TraceLog(LOG_WARNING, "Despawn stale enemy %u", enemy.id);
            //    RemoveSlime(enemy.id);
            //    continue;
            //}
        }
    }

    itemSystem.DespawnDeadEntities();
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
    if (!map || zoomMipLevel <= 0) {
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

            const Tile *tile = map->TileAtWorld(xx, yy);
            if (tile) {
                tileset_draw_tile(map->tilesetId, tile->type, { xx, yy }, WHITE);

                //uint8_t r = tile->base;
                //uint8_t g = tile->baseNoise;
                //const int xxi = (int)xx;
                //const int yyi = (int)yy;
                //DrawRectangle(xxi    , yyi     , SUBTILE_W, SUBTILE_H, { r, 0, 0, 255 });  // Top left
                //DrawRectangle(xxi + 8, yyi     , SUBTILE_W, SUBTILE_H, { 0, g, 0, 255 });  // Top right

                //DrawRectangle(xxi    , yyi + 16, SUBTILE_W, SUBTILE_H, { 0, 0, 0, 255 });  // Bottom left
                //DrawRectangle(xxi + 8, yyi + 16, SUBTILE_W, SUBTILE_H, { 0, 0, 0, 255 });  // Bottom right
            } else {
                tileset_draw_tile(map->tilesetId, TileType_Void, { xx, yy }, WHITE);
            }
#if 0
            const Tile *tile = map->TileAtWorld(xx, yy);
            Tileset &tileset = g_tilesets[(size_t)map->tilesetId];
            Rectangle tileRect = tileset_tile_rect(map->tilesetId, tile ? tile->type : TileType_Void);

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

    return tilesDrawn;
}

void World::DrawItems(void)
{
    itemSystem.PushAll(drawList);
}

void World::DrawEntities(void)
{
    for (const Player &player : players) {
        if (player.id) {
            drawList.Push(player);
        }
    }

    for (const Slime &slime : slimes) {
        if (slime.id) {
            drawList.Push(slime);
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