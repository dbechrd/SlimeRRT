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
        0.0f
    };
    return worldSpawn;
};

Player *World::SpawnPlayer(uint32_t playerId)
{
#if _DEBUG
    if (playerId) {
        for (int i = 0; i < SV_MAX_PLAYERS; i++) {
            Player &player = players[i];
            if (player.id == playerId) {
                assert(!"This playerId is already in use!");
            }
        }
    }
#endif

    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (player.id == 0) {
            assert(!player.nameLength);
            assert(!player.combat.hitPointsMax);

            if (playerId) {
                player.id = playerId;
            } else {
                static uint32_t nextId = 0;
                nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
                player.id = nextId;
            }

            const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Charlie);
            const SpriteDef *spriteDef = spritesheet.FindSprite("player_sword");
            player.Init(spriteDef);
            player.body.position = GetWorldSpawn();

            return &player;
        }
    }
    return 0;
}

Player *World::FindPlayer(uint32_t playerId)
{
    if (!playerId) {
        return 0;
    }

    for (int i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (player.id == playerId) {
            return &player;
        }
    }
    return 0;
}

Player *World::FindClosestPlayer(Vector2 worldPos, float maxDist)
{
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        if (players[i].id && players[i].combat.hitPoints > 0) {
            Vector2 toPlayer = v2_sub(players[i].body.GroundPosition(), worldPos);
            const float toPlayerDistSq = v2_length_sq(toPlayer);
            if (toPlayerDistSq <= SQUARED(maxDist)) {
                return &players[i];
            }
        }
    }
    return 0;
}

void World::DespawnPlayer(uint32_t playerId)
{
    Player *player = FindPlayer(playerId);
    if (player) {
        printf("Despawn player %u\n", playerId);
        *player = {};
    }
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
    sam.body.position = v3_add(GetWorldSpawn(), { 0, -300.0f, 0 });
    sam.sprite.scale = 2.0f;
    return sam;
}

Slime *World::SpawnSlime(uint32_t slimeId)
{
#if _DEBUG
    if (slimeId) {
        for (int i = 0; i < SV_MAX_ENTITIES; i++) {
            Slime &slime = slimes[i];
            if (slime.id == slimeId) {
                assert(!"This slimeId is already in use!");
            }
        }
    }
#endif

    for (int i = 0; i < SV_MAX_ENTITIES; i++) {
        Slime &slime = slimes[i];
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
            slime.body.position.x = dlb_rand32f_range(slimeRadius, maxX);
            slime.body.position.y = dlb_rand32f_range(slimeRadius, maxY);

            return &slime;
        }
    }
    return 0;
}

Slime *World::FindSlime(uint32_t slimeId)
{
    if (!slimeId) {
        return 0;
    }

    for (int i = 0; i < SV_MAX_ENTITIES; i++) {
        Slime &slime = slimes[i];
        if (slime.id == slimeId) {
            return &slime;
        }
    }
    return 0;
}

void World::DespawnSlime(uint32_t slimeId)
{
    Slime *slime = FindSlime(slimeId);
    if (slime) {
        if (!slime->combat.hitPoints) {
            const Vector3 slimeBC = sprite_world_center(slime->sprite, slime->body.position, slime->sprite.scale);
            particleSystem.GenerateEffect(Catalog::ParticleEffectID::Goo, 20, slimeBC, 2.0);
            Catalog::g_sounds.Play(Catalog::SoundID::Squish2, 0.5f + dlb_rand32f_variance(0.1f), true);
        }
        printf("Despawn slime %u\n", slimeId);
        *slime = {};
    }
}

void BloodParticlesFollowPlayer(ParticleEffect &effect, void *userData)
{
    assert(effect.id == Catalog::ParticleEffectID::Blood);
    assert(userData);

    Player *player = (Player *)userData;
    effect.origin = player->GetAttachPoint(Player::AttachPoint::Gut);
}

void World::Simulate(double dt)
{
    SimPlayers(dt);
    SimSlimes(dt);
    SimItems(dt);
}

void World::SimPlayers(double dt)
{
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (!player.id) {
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

            size_t slimesHit = 0;
            for (Slime &slime : slimes) {
                if (!slime.id) {
                    continue;
                }

                Vector3 playerToSlime = v3_sub(slime.body.position, player.body.position);
                if (v3_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                    player.stats.damageDealt += CLAMP(playerDamage, 0.0f, slime.combat.hitPoints);
                    slime.combat.hitPoints = CLAMP(
                        slime.combat.hitPoints - playerDamage,
                        0.0f,
                        slime.combat.hitPointsMax
                    );
                    if (!slime.combat.hitPoints) {
                        uint32_t coins = lootSystem.RollCoins(slime.combat.lootTableId, (int)slime.sprite.scale);
                        assert(coins);

                        Vector3 itemPos = slime.body.position;
                        itemPos.x += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
                        itemPos.y += dlb_rand32f_variance(METERS_TO_PIXELS(0.5f));
                        itemSystem.SpawnItem(itemPos, Catalog::ItemID::Currency_Coin, coins);

                        DespawnSlime(slime.id);
                        player.stats.slimesSlain++;
                    } else {
                        Catalog::g_sounds.Play(Catalog::SoundID::Slime_Stab1, 1.0f + dlb_rand32f_variance(0.4f));
                    }
                    slimesHit++;
                }
            }
        }
    }
}

void World::SimSlimes(double dt)
{
    // TODO: Move these to somewhere
    const float slimeMoveSpeed = METERS_TO_PIXELS(2.0f);
    const float slimeAttackReach = METERS_TO_PIXELS(0.5f);
    const float slimeAttackTrack = METERS_TO_PIXELS(10.0f);
    const float slimeRadius = METERS_TO_PIXELS(0.5f);

    for (size_t slimeIdx = 0; slimeIdx < SV_MAX_ENTITIES; slimeIdx++) {
        Slime &slime = slimes[slimeIdx];
        if (!slime.id) {
            continue;
        }

        // TODO: Actually find closest alive player via RTree
        Player *closestPlayer = FindClosestPlayer(slime.body.GroundPosition(), slimeAttackTrack);
        if (!closestPlayer || !closestPlayer->id) {
            slime.Update(dt);
            continue;
        }

        Vector2 slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), slime.body.GroundPosition());
        const float slimeToPlayerDistSq = v2_length_sq(slimeToPlayer);
        if (slimeToPlayerDistSq <= SQUARED(slimeAttackTrack)) {
            const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
            const float moveDist = MIN(slimeToPlayerDist, slimeMoveSpeed * slime.sprite.scale);
            // 5% -1.0, 95% +1.0f
            const float moveRandMult = 1.0f; //dlb_rand32i_range(1, 100) > 5 ? 1.0f : -1.0f;
            const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
            const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
            const Vector2 slimePos = slime.body.GroundPosition();
            const Vector2 slimePosNew = v2_add(slimePos, slimeMove);

            int willCollide = 0;
            for (size_t collideIdx = slimeIdx + 1; collideIdx < SV_MAX_ENTITIES; collideIdx++) {
                Slime &otherSlime = slimes[collideIdx];
                if (!otherSlime.id) {
                    continue;
                }

                Vector2 otherSlimePos = otherSlime.body.GroundPosition();
                const float zDist = fabsf(slime.body.position.z - otherSlime.body.position.z);
                const float radiusScaled = slimeRadius * slime.sprite.scale;
                if (v2_length_sq(v2_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                    if (slime.Combine(otherSlime)) {
                        const Slime &dead = slime.combat.hitPoints == 0.0f ? slime : otherSlime;
                        DespawnSlime(dead.id);
                    }
                }
                if (v2_length_sq(v2_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
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
        if (v2_length_sq(slimeToPlayer) <= SQUARED(slimeAttackReach)) {
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


void World::SimItems(double dt)
{
    // TODO: Move these to somewhere
    const float playerItemPickupReach = METERS_TO_PIXELS(1.5f);
    const float playerItemPickupDist = METERS_TO_PIXELS(0.1f);

    const size_t itemCount = itemSystem.ItemsActive();
    ItemWorld *items = itemSystem.Items();

    for (size_t itemIdx = 0; itemIdx < itemCount; itemIdx++) {
        ItemWorld &item = items[itemIdx];
        assert(item.stack.id != Catalog::ItemID::Empty);

        // TODO: Actually find closest alive player via RTree
        Player *closestPlayer = FindClosestPlayer(item.body.GroundPosition(), playerItemPickupReach);
        if (!closestPlayer || !closestPlayer->id) {
            continue;
        }

        Vector3 itemToPlayer = v3_sub(closestPlayer->body.position, item.body.position);
        const float itemToPlayerDistSq = v3_length_sq(itemToPlayer);
        if (itemToPlayerDistSq < SQUARED(playerItemPickupDist)) {
            item.pickedUp = true;

            switch (item.stack.id) {
                case Catalog::ItemID::Currency_Coin: {
                    // TODO(design): Convert coins to higher currency if stack fills up?
                    closestPlayer->inventory.slots[(int)PlayerInventorySlot::Coins].count += item.stack.count;
                    closestPlayer->stats.coinsCollected += item.stack.count;
                    Catalog::g_sounds.Play(Catalog::SoundID::Gold, 1.0f + dlb_rand32f_variance(0.2f), true);
                    break;
                }
            }
        } else {
            const Vector3 itemToPlayerDir = v3_normalize(itemToPlayer);
            const float speed = MAX(0, 1.0f / PIXELS_TO_METERS(sqrtf(itemToPlayerDistSq)));
            const Vector3 itemVel = v3_scale(itemToPlayerDir, METERS_TO_PIXELS(speed));
            item.body.velocity.x = itemVel.x;
            item.body.velocity.y = itemVel.y;
            //item.body.velocity.z = MAX(item.body.velocity.z, itemVel.z);
        }
    }

    itemSystem.Update(dt);
}

void World::GenerateSnapshot(WorldSnapshot &worldSnapshot)
{
    assert(worldSnapshot.playerId);
    assert(worldSnapshot.tick);

    assert(ARRAY_SIZE(worldSnapshot.players) <= ARRAY_SIZE(players));
    assert(ARRAY_SIZE(worldSnapshot.slimes)  <= ARRAY_SIZE(slimes));
    assert(ARRAY_SIZE(worldSnapshot.players) == SNAPSHOT_MAX_PLAYERS);
    assert(ARRAY_SIZE(worldSnapshot.slimes)  == SNAPSHOT_MAX_ENTITIES);

    Player *player = FindPlayer(worldSnapshot.playerId);
    if (!player) {
        return;
    }

    // TODO: Find players/slimes/etc. that are actually near the player this snapshot is being generated for
    worldSnapshot.playerCount = 0;
    for (size_t i = 0; i < SV_MAX_PLAYERS && worldSnapshot.playerCount < SNAPSHOT_MAX_PLAYERS; i++) {
        worldSnapshot.players[worldSnapshot.playerCount].id           = players[i].id                 ;
        worldSnapshot.players[worldSnapshot.playerCount].nameLength   = players[i].nameLength         ;
        memcpy(worldSnapshot.players[i].name, players[i].name, USERNAME_LENGTH_MAX);
        if (players[i].nameLength) {
            assert(worldSnapshot.players[i].name[0]);
        }
        worldSnapshot.players[worldSnapshot.playerCount].position     = players[i].body.position      ;
        worldSnapshot.players[worldSnapshot.playerCount].direction    = players[i].sprite.direction   ;
        worldSnapshot.players[worldSnapshot.playerCount].hitPoints    = players[i].combat.hitPoints   ;
        worldSnapshot.players[worldSnapshot.playerCount].hitPointsMax = players[i].combat.hitPointsMax;
        worldSnapshot.playerCount++;
    }
    worldSnapshot.slimeCount = 0;
    for (size_t i = 0; i < SV_MAX_ENTITIES && worldSnapshot.slimeCount < SNAPSHOT_MAX_ENTITIES; i++) {
        if (v3_length_sq(v3_sub(player->body.position, slimes[i].body.position)) < SQUARED(1300.0f)) {
            worldSnapshot.slimes[worldSnapshot.slimeCount].id           = slimes[i].id                 ;
            worldSnapshot.slimes[worldSnapshot.slimeCount].position     = slimes[i].body.position      ;
            worldSnapshot.slimes[worldSnapshot.slimeCount].direction    = slimes[i].sprite.direction   ;
            worldSnapshot.slimes[worldSnapshot.slimeCount].hitPoints    = slimes[i].combat.hitPoints   ;
            worldSnapshot.slimes[worldSnapshot.slimeCount].hitPointsMax = slimes[i].combat.hitPointsMax;
            worldSnapshot.slimes[worldSnapshot.slimeCount].scale        = slimes[i].sprite.scale       ;
            worldSnapshot.slimeCount++;
        }
    }
}

bool World::InterpolateBody(Body3D &body, double renderAt, Direction &direction)
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
        printf("renderAt %f before oldest snapshot %f\n", renderAt, oldest.recvAt);

        body.position = oldest.v;
        direction = oldest.direction;
    // renderAt is after all snapshots, show entity at newest snapshot
    } else if (right == historyLen) {
        // TODO: Extrapolate beyond latest snapshot if/when this happens? Should be mostly avoidable..
        Vector3Snapshot &newest = positionHistory.At(historyLen - 1);

        assert(renderAt >= newest.recvAt);
        if (renderAt > newest.recvAt) {
            printf("renderAt %f after newest snapshot %f\n", renderAt, newest.recvAt);
        }

        // TODO: Send explicitly despawn event from server
        // If we haven't seen an entity in 2 snapshots, chances are it's gone
        if (renderAt > newest.recvAt + (1.0 / SNAPSHOT_SEND_RATE) * 2) {
            printf("Despawning body due to inactivity\n");
            return false;
        }

        body.position = newest.v;
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
        body.position = v3_add(a.v, v3_scale(v3_sub(b.v, a.v), (float)alpha));
        direction = b.direction;
    }

    return true;
}

void World::Interpolate(double renderAt)
{
    for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (!player.id || player.id == playerId) {
            continue;
        }
        Direction direction{};
        if (InterpolateBody(player.body, renderAt, direction)) {
            player.sprite.direction = direction;
        } else {
            DespawnPlayer(player.id);
        }
    }
    for (size_t i = 0; i < SV_MAX_ENTITIES; i++) {
        Slime &slime = slimes[i];
        if (!slime.id) {
            continue;
        }
        Direction direction{};
        if (slime.combat.hitPoints && InterpolateBody(slime.body, renderAt, direction)) {
            slime.sprite.direction = direction;
        } else {
            DespawnSlime(slime.id);
        }
    }
    return;
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
                Rectangle textureRect = tileset_tile_rect(map->tilesetId, tile->tileType);
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
        if (slime.combat.hitPoints) {
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