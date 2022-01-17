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
    Vector3 worldSpawn = {
        (float)map->width / 2.0f * 32,
        (float)map->height / 2.0f * 32,
        0
    };
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

Player *World::FindClosestPlayer(Vector2 worldPos, float maxDist)
{
    for (Player &player : players) {
        if (player.id && player.combat.hitPoints) {
            Vector2 toPlayer = v2_sub(player.body.GroundPosition(), worldPos);
            const float toPlayerDistSq = v2_length_sq(toPlayer);
            if (toPlayerDistSq <= SQUARED(maxDist)) {
                return &player;
            }
        }
    }
    return 0;
}

#if 0
void World::DespawnPlayer(uint32_t playerId)
{
    Player *player = FindPlayer(playerId);
    if (!player) {
        TraceLog(LOG_ERROR, "Cannot visually despawn a player that doesn't exist. playerId: %u", playerId);
        return;
    }

    player->sprite.spriteDef = 0;
}
#endif

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
    Slime *slime = SpawnSlime(0);
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

Slime *World::SpawnSlime(uint32_t slimeId)
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

            if (slimeId) {
                slime.id = slimeId;
            } else {
                static uint32_t nextId = 0;
                nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
                slime.id = nextId;
            }

            slime.Init();

            // TODO: Move slime radius somewhere more logical.. some global table of magic numbers?
            // Or.. use sprite size as radius
            // Or.. implement a more general "place entity" solution for rocks, trees, monsters, etc.
            const float slimeRadius = 50.0f;
            const size_t mapPixelsX = (size_t)map->width * TILE_W;
            const size_t mapPixelsY = (size_t)map->height * TILE_W;
            const float maxX = mapPixelsX - slimeRadius;
            const float maxY = mapPixelsY - slimeRadius;
            slime.body.Teleport({
                dlb_rand32f_range(slimeRadius, maxX),
                dlb_rand32f_range(slimeRadius, maxY),
                0.0f
            });

            //TraceLog(LOG_DEBUG, "Spawned enemy %u", slime.id);
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

void World::DespawnSlime(uint32_t slimeId)
{
    Slime *slime = FindSlime(slimeId);
    if (!slime) {
        TraceLog(LOG_ERROR, "Cannot remove a slime that doesn't exist. slimeId: %u", slimeId);
        return;
    }

    //TraceLog(LOG_DEBUG, "Despawn enemy %u", slimeId);
    *slime = {};
}

void BloodParticlesFollowPlayer(ParticleEffect &effect, void *userData)
{
    assert(effect.id == Catalog::ParticleEffectID::Blood);
    assert(userData);

    Player *player = (Player *)userData;
    effect.origin = player->GetAttachPoint(Player::AttachPoint::Gut);
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
        if (!player.id || player.combat.diedAt) {
            continue;
        }

        //assert(map);
        //player.Update(dt, *map);

        if (player.actionState == Player::ActionState::Attacking && player.combat.attackFrame == 1) {
            const float playerAttackReach = METERS_TO_PIXELS(1.0f);
            float playerDamage;

            const ItemStack &selectedStack = player.GetSelectedItem();
            const Catalog::Item &selectedItem = Catalog::g_items.FindById(selectedStack.id);
            playerDamage = selectedItem.damage;
            if (selectedItem.id == Catalog::ItemID::Empty) {
                playerDamage = player.combat.meleeDamage;
            }

            for (Slime &slime : slimes) {
                if (!slime.id || slime.combat.diedAt) {
                    continue;
                }

                Vector3 playerToSlime = v3_sub(slime.body.WorldPosition(), player.body.WorldPosition());
                if (v3_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                    player.stats.damageDealt += CLAMP(playerDamage, 0.0f, slime.combat.hitPoints);
                    slime.combat.hitPoints = CLAMP(
                        slime.combat.hitPoints - playerDamage,
                        0.0f,
                        slime.combat.hitPointsMax
                    );
                    if (!slime.combat.hitPoints) {
                        player.stats.slimesSlain++;
                    } else {
                        Catalog::g_sounds.Play(Catalog::SoundID::Slime_Stab1, 1.0f + dlb_rand32f_variance(0.4f));
                    }
                }
            }
        }
    }
}

void World::SV_SimSlimes(double dt)
{
    for (size_t slimeIdx = 0; slimeIdx < SV_MAX_SLIMES; slimeIdx++) {
        Slime &slime = slimes[slimeIdx];
        if (!slime.id || slime.combat.diedAt) {
            continue;
        }

        if (!slime.combat.hitPoints) {
            uint32_t coins = lootSystem.RollCoins(slime.combat.lootTableId, (int)slime.sprite.scale);
            assert(coins);

            Vector3 itemPos = slime.WorldCenter();
            itemPos.x += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
            itemPos.y += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
            itemSystem.SpawnItem(itemPos, Catalog::ItemID::Currency_Coin, coins);

            slime.combat.diedAt = glfwGetTime();
            continue;
        }

        // TODO: Actually find closest alive player via RTree
        Player *closestPlayer = FindClosestPlayer(slime.body.GroundPosition(), SV_SLIME_ATTACK_TRACK);
        if (!closestPlayer || !closestPlayer->id) {
            slime.Update(dt);
            continue;
        }

        Vector2 slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), slime.body.GroundPosition());
        const float slimeToPlayerDistSq = v2_length_sq(slimeToPlayer);
        if (slimeToPlayerDistSq <= SQUARED(SV_SLIME_ATTACK_TRACK)) {
            const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
            const float moveDist = MIN(slimeToPlayerDist, SV_SLIME_MOVE_SPEED * slime.sprite.scale);
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
                //Catalog::SoundID squish = dlb_rand32i_range(0, 1) ? Catalog::SoundID::Squish1 : Catalog::SoundID::Squish2;
                Catalog::SoundID squish = Catalog::SoundID::Squish1;
                Catalog::g_sounds.Play(squish, 1.0f + dlb_rand32f_variance(0.2f), true);
            }
        }

        // Allow slime to attack if on the ground and close enough to the player
        slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), slime.body.GroundPosition());
        if (v2_length_sq(slimeToPlayer) <= SQUARED(SV_SLIME_ATTACK_REACH)) {
            if (slime.Attack(dt)) {
                closestPlayer->combat.hitPoints = CLAMP(
                    closestPlayer->combat.hitPoints - (slime.combat.meleeDamage * slime.sprite.scale),
                    0.0f,
                    closestPlayer->combat.hitPointsMax
                );

                static double lastBleed = 0;
                const double bleedDuration = 1.0;

                if (glfwGetTime() - lastBleed > bleedDuration / 3.0) {
                    Vector3 playerGut = closestPlayer->GetAttachPoint(Player::AttachPoint::Gut);
                    ParticleEffect *bloodParticles = particleSystem.GenerateEffect(Catalog::ParticleEffectID::Blood, 32, playerGut, bleedDuration);
                    if (bloodParticles) {
                        bloodParticles->callbacks[(size_t)ParticleEffectEvent::BeforeUpdate].function = BloodParticlesFollowPlayer;
                        bloodParticles->callbacks[(size_t)ParticleEffectEvent::BeforeUpdate].userData = closestPlayer;
                    }
                    lastBleed = glfwGetTime();
                }
            }
        }

        slime.Update(dt);
    }
}

void World::SV_SimItems(double dt)
{
    // TODO: Move these to somewhere
    const float playerItemPickupReach = METERS_TO_PIXELS(1.5f);
    const float playerItemPickupDist = METERS_TO_PIXELS(0.1f);

    for (ItemWorld &item : itemSystem.items) {
        if (!item.id || item.pickedUpAt) {
            continue;
        }

        assert(item.stack.id != Catalog::ItemID::Empty);

        // TODO: Actually find closest alive player via RTree
        Player *closestPlayer = FindClosestPlayer(item.body.GroundPosition(), playerItemPickupReach);
        if (!closestPlayer || !closestPlayer->id) {
            continue;
        }

        Vector2 itemToPlayer = v2_sub(closestPlayer->body.GroundPosition(), item.body.GroundPosition());
        const float itemToPlayerDistSq = v2_length_sq(itemToPlayer);
        if (itemToPlayerDistSq < SQUARED(playerItemPickupDist)) {
            item.pickedUpAt = glfwGetTime();
            TraceLog(LOG_DEBUG, "Sim: Item picked up %u", item.id);

            switch (item.stack.id) {
                case Catalog::ItemID::Currency_Coin: {
                    // TODO(design): Convert coins to higher currency if stack fills up?
                    closestPlayer->inventory.slots[(int)PlayerInventorySlot::Coins].count += item.stack.count;
                    closestPlayer->stats.coinsCollected += item.stack.count;
                    break;
                }
            }
        } else {
            const Vector2 itemToPlayerDir = v2_normalize(itemToPlayer);
            const float speed = MAX(0, 1.0f / PIXELS_TO_METERS(sqrtf(itemToPlayerDistSq)));
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
        if (!player.id) {
            continue;
        }

        // NOTE: Player is likely going to respawn very quickly, so this may not be useful
        if (!player.combat.hitPoints && glfwGetTime() - player.combat.diedAt > SV_PLAYER_CORPSE_LIFETIME) {
            TraceLog(LOG_DEBUG, "Despawn stale player corpse %u", player.id);
            DespawnPlayer(player.id);
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
            DespawnSlime(enemy.id);
        }
    }

    itemSystem.DespawnDeadEntities();
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
        if (!item.id) {
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
        player.Update(dt, input, *map);
    }
    for (Slime &enemy : slimes) {
        if (!enemy.id) {
            continue;
        }
        enemy.Update(dt);
    }
    for (ItemWorld &item : itemSystem.items) {
        if (!item.id) {
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
        if (!otherPlayer.id || otherPlayer.id == playerId) {
            continue;
        }

        const float distSq = v3_length_sq(v3_sub(player->body.WorldPosition(), otherPlayer.body.WorldPosition()));
        const bool faraway = distSq >= SQUARED(CL_PLAYER_FARAWAY_THRESHOLD);
        if (faraway) {
            TraceLog(LOG_DEBUG, "Despawn far away player %u", otherPlayer.id);
            DespawnPlayer(otherPlayer.id);
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
                //TraceLog(LOG_DEBUG, "Despawn far away enemy %u", enemy.id);
                DespawnSlime(enemy.id);
                continue;
            }
        }
    }

    itemSystem.CL_DespawnStaleEntities();
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

size_t World::DrawMap(int zoomMipLevel)
{
    assert(zoomMipLevel > 0);
    if (!map || !map->tiles) return 0;

    size_t tilesDrawn = 0;
    for (size_t y = 0; y < map->height; y += zoomMipLevel) {
        for (size_t x = 0; x < map->width; x += zoomMipLevel) {
            const Tile *tile = &map->tiles[y * map->width + x];
            const Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
            if (!CullTile(tilePos, zoomMipLevel)) {
                // Draw all tiles as textured rects (looks best, performs worst)
                tileset_draw_tile(map->tilesetId, tile->tileType, tilePos);
                tilesDrawn++;
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
    drawList.Flush();
}