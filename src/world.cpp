#include "world.h"
#include "body.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "controller.h"
#include "direction.h"
#include "helpers.h"
#include "maths.h"
#include "monster/monster.h"
#include "particles.h"
#include "player.h"
#include "enemy.h"
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

    for (int i = 0; i < ARRAY_SIZE(playerInfos); i++) {
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
    PlayerInfo *playerInfo = FindPlayerInfo(playerId);
    if (!playerInfo) return;

    *playerInfo = {};
}

Player *World::AddPlayer(uint32_t playerId)
{
    Player *existingPlayer = FindPlayer(playerId);
    if (existingPlayer) {
        //TraceLog(LOG_WARNING, "This playerId is already in use!");
        //return existingPlayer;
        return 0;
    }

    const PlayerInfo *playerInfo = FindPlayerInfo(playerId);
    assert(playerInfo->nameLength);

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

Player *World::LocalPlayer(void)
{
    return FindPlayer(playerId);
}

Player *World::FindPlayerByName(const char *name, size_t nameLength)
{
    PlayerInfo *playerInfo = FindPlayerInfoByName(name, nameLength);
    return playerInfo ? FindPlayer(playerInfo->id) : 0;
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

Enemy &World::SpawnSam(void)
{
    Enemy *enemy = SpawnEnemy(0, {0, 0});
    assert(enemy);
    Enemy &sam = *enemy;
    sam.SetName(CSTR("Sam"));
    sam.combat.hitPointsMax = 20.0f; //100000.0f;
    sam.combat.hitPoints = sam.combat.hitPointsMax;
    sam.combat.meleeDamage = -1.0f;
    sam.combat.lootTableId = LootTableID::LT_Sam;
    sam.body.Teleport(v3_add(GetWorldSpawn(), { -200.0f, 0, 0 }));
    sam.sprite.scale = 2.0f;
    return sam;
}

Enemy *World::SpawnEnemy(uint32_t enemyId, Vector2 origin)
{
#if _DEBUG
    if (enemyId) {
        for (Enemy &enemy : enemies) {
            if (enemy.id == enemyId) {
                assert(!"This enemyId is already in use!");
            }
        }
    }
#endif

    for (Enemy &enemy : enemies) {
        if (enemy.id == 0) {
            assert(!enemy.nameLength);
            assert(!enemy.combat.hitPointsMax);

            Vector3 spawnPos{};

            if (g_clock.server) {
                spawnPos.x = dlb_rand32f_variance(1.0f);
                spawnPos.y = dlb_rand32f_variance(1.0f);
                spawnPos = v3_normalize(spawnPos);

                // Scale into correct range for valid spawn ring
                float mult = dlb_rand32f_range(SV_ENEMY_MIN_SPAWN_DIST, SV_ENEMY_DESPAWN_RADIUS);
                spawnPos = v3_scale(spawnPos, mult);

                // Translate to whatever point we want to spawn
                spawnPos.x += origin.x;
                spawnPos.y += origin.y;

                const Tile *spawnTile = map.TileAtWorld(spawnPos.x, spawnPos.y);
                if (!spawnTile || !spawnTile->IsSpawnable()) {
                    return 0;
                }
            }
#if 0
            for (Player &player : players) {
                float spawnDist = v3_length_sq(v3_sub(spawnPos, player.body.WorldPosition()));
                if (spawnDist < SV_ENEMY_MIN_SPAWN_DIST * SV_ENEMY_MIN_SPAWN_DIST) {
                    //TraceLog(LOG_DEBUG, "Failed to spawn enemy, too close to player");
                    //return 0;
                }
            }
#endif
            if (enemyId) {
                enemy.id = enemyId;
            } else {
                thread_local uint32_t nextId = 0;
                nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
                enemy.id = nextId;
            }

            Monster::slime_init(enemy);
            if (g_clock.server) {
                enemy.body.Teleport(spawnPos);
            }
            //TraceLog(LOG_DEBUG, "SpawnEnemy [%u]", enemy.id);
            return &enemy;
        }
    }

    //TraceLog(LOG_DEBUG, "Failed to spawn enemy, mob cap full");
    return 0;
}

Enemy *World::FindEnemy(uint32_t enemyId)
{
    if (!enemyId) {
        return 0;
    }

    for (Enemy &enemy : enemies) {
        if (enemy.id == enemyId) {
            return &enemy;
        }
    }
    return 0;
}

void World::RemoveEnemy(uint32_t enemyId)
{
    Enemy *enemy = FindEnemy(enemyId);
    if (!enemy) {
        //TraceLog(LOG_ERROR, "Cannot remove a enemy that doesn't exist. enemyId: %u", enemyId);
        return;
    }

    //TraceLog(LOG_DEBUG, "RemoveEnemy [%u]", enemy->id);
    *enemy = {};
}

void World::SV_Simulate(double dt)
{
    SV_SimPlayers(dt);
    SV_SimEnemies(dt);
    SV_SimItems(dt);
}

void World::SV_SimPlayers(double dt)
{
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

            for (Enemy &enemy : enemies) {
                if (!enemy.id || enemy.combat.diedAt) {
                    continue;
                }

                Vector3 playerToEnemy = v3_sub(enemy.body.WorldPosition(), player.body.WorldPosition());
                if (v3_length_sq(playerToEnemy) <= SQUARED(playerAttackReach)) {
                    player.stats.damageDealt += enemy.combat.DealDamage(playerDamage);
                    if (playerKnockback) {
                        Vector3 knockbackDir = v3_normalize(playerToEnemy);
                        Vector3 knockbackVec = v3_scale(knockbackDir, METERS_TO_PIXELS(playerKnockback));
                        enemy.body.ApplyForce(knockbackVec);
                    }
                    if (!enemy.combat.hitPoints) {
                        player.xp += dlb_rand32u_range(enemy.combat.xpMin, enemy.combat.xpMax);
                        int overflowXp = player.xp - (player.combat.level * 20u);
                        if (overflowXp >= 0) {
                            player.combat.level++;
                            player.xp = (uint32_t)overflowXp;
                        }
                        player.stats.enemiesSlain[enemy.type]++;
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
        }

        // Try to spawn enemies near player
        if (!peaceful && dlb_rand32f() < 0.9f) {
            SpawnEnemy(0, player.body.GroundPosition());
        }
    }
}

void World::SV_SimEnemies(double dt)
{
    for (size_t enemyIdx = 0; enemyIdx < SV_MAX_ENEMIES; enemyIdx++) {
        Enemy &enemy = enemies[enemyIdx];
        if (!enemy.id) {
            continue;
        }
        enemy.Update(*this, dt);
    }
}

void World::SV_SimItems(double dt)
{
    for (ItemWorld &item : itemSystem.worldItems) {
        if (!item.euid || item.pickedUpAt || g_clock.now < item.spawnedAt + SV_ITEM_PICKUP_DELAY) {
            continue;
        }

        assert(item.stack.uid);
        assert(item.stack.count);

        Player *closestPlayer = FindClosestPlayer(item.body.GroundPosition(), SV_ITEM_ATTRACT_DIST);
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
                    item.pickedUpAt = g_clock.now;
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
#if 0
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (!player.type) {
            continue;
        }

        // NOTE: Player is likely going to respawn very quickly, so this may not be useful
        if (!player.combat.hitPoints && g_clock.now - player.combat.diedAt > SV_PLAYER_CORPSE_LIFETIME) {
            TraceLog(LOG_DEBUG, "Despawn stale player corpse %u", player.type);
            DespawnPlayer(player.type);
        }
    }
#endif

    for (const Enemy &enemy : enemies) {
        if (!enemy.id) {
            continue;
        }

        // Check if enemy has been dead for awhile
        if (enemy.combat.diedAt && g_clock.now - enemy.combat.diedAt > SV_ENEMY_CORPSE_LIFETIME) {
            //TraceLog(LOG_DEBUG, "Despawn stale enemy corpse %u", enemy.id);
            RemoveEnemy(enemy.id);
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
        const Vector3Snapshot &oldest = positionHistory.At(0);
        assert(renderAt < oldest.recvAt);
        //printf("renderAt %f before oldest snapshot %f\n", renderAt, oldest.recvAt);

        body.Teleport(oldest.v);
        direction = oldest.direction;
    // renderAt is after all snapshots, show entity at newest snapshot
    } else if (right == historyLen) {
        // TODO: Extrapolate beyond latest snapshot if/when this happens? Should be mostly avoidable..
        const Vector3Snapshot &newest = positionHistory.At(historyLen - 1);
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
        const Vector3Snapshot &a = positionHistory.At(right - 1);
        const Vector3Snapshot &b = positionHistory.At(right);

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
    for (Enemy &enemy : enemies) {
        if (!enemy.id) {
            continue;
        }
        CL_InterpolateBody(enemy.body, renderAt, enemy.sprite.direction);
    }
    for (ItemWorld &item : itemSystem.worldItems) {
        if (!item.euid) {
            continue;
        }
        CL_InterpolateBody(item.body, renderAt, item.sprite.direction);
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
    for (Enemy &enemy : enemies) {
        if (!enemy.id) {
            continue;
        }
        enemy.Update(*this, dt);
    }
    for (ItemWorld &item : itemSystem.worldItems) {
        if (!item.euid || item.pickedUpAt || g_clock.now < item.spawnedAt + SV_ITEM_PICKUP_DELAY) {
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
    for (Enemy &enemy : enemies) {
        if (!enemy.id) {
            continue;
        }
        enemy.combat.Update(dt);
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

    for (const Enemy &enemy : enemies) {
        if (!enemy.id) {
            continue;
        }

        if (enemy.body.positionHistory.Count()) {
            auto &lastPos = enemy.body.positionHistory.Last();

            const float distSq = v3_length_sq(v3_sub(player->body.WorldPosition(), lastPos.v));
            const bool faraway = distSq >= SQUARED(CL_ENEMY_FARAWAY_THRESHOLD);
            if (faraway) {
                //TraceLog(LOG_DEBUG, "Despawn far away enemy %u", enemy.type);
                RemoveEnemy(enemy.id);
                continue;
            }

            // TODO: This hack doesn't work since we're only sending snapshots when a slime moves, and slimes don't
            // move unless they have a target. Need to figure out why slime is just chillin and not updating.
            // HACK: Figure out WHY this is happening instead of just despawning. I'm assuming the server just didn't
            // send me a despawn packet for this slime (e.g. because I'm too far away from it??)
            //if (g_clock.now - enemy.body.positionHistory.Last().recvAt > SV_ENEMY_STALE_LIFETIME) {
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

    for (Enemy &slime : enemies) {
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