#include "world.h"
#include "body.h"
#include "controller.h"
#include "helpers.h"
#include "maths.h"
#include "particles.h"
#include "player.h"
#include "player_inventory.h"
#include "slime.h"
#include "sound_catalog.h"
#include "spritesheet.h"
#include "spritesheet_catalog.h"
#include "tilemap.h"
#include "world.h"
#include "dlb_rand.h"
#include <cassert>

World::World(void)
{
    rtt_seed = 16; //time(NULL);
    dlb_rand32_seed_r(&rtt_rand, rtt_seed, rtt_seed);
}

World::~World(void)
{
}

const Vector3 World::GetWorldSpawn(void)
{
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
        for (int i = 0; i < SERVER_PLAYERS_MAX; i++) {
            Player &player = players[i];
            if (player.id == playerId) {
                assert(!"This playerId is already in use!");
            }
        }
    }
#endif

    for (int i = 0; i < SERVER_PLAYERS_MAX; i++) {
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

            player.Init();
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

    for (int i = 0; i < SERVER_PLAYERS_MAX; i++) {
        Player &player = players[i];
        if (player.id == playerId) {
            return &player;
        }
    }
    return 0;
}

void World::DespawnPlayer(uint32_t playerId)
{
    Player *player = FindPlayer(playerId);
    if (player) {
        printf("Despawn player %u\n", playerId);
        memset(player, 0, sizeof(*player));
    }
}

void World::SpawnSam(void)
{
    Slime *slime = SpawnSlime(0);
    assert(slime);
    Slime &sam = *slime;
    sam.SetName(CSTR("Sam"));
    sam.combat.hitPointsMax = 100.0f; //100000.0f;
    sam.combat.hitPoints = sam.combat.hitPointsMax;
    sam.combat.meleeDamage = -1.0f;
    sam.combat.lootTableId = LootTableID::LT_Sam;
    sam.body.position = v3_add(GetWorldSpawn(), { 0, -300.0f, 0 });
    sam.sprite.scale = 2.0f;
}

Slime *World::SpawnSlime(uint32_t slimeId)
{
#if _DEBUG
    if (slimeId) {
        for (int i = 0; i < WORLD_ENTITIES_MAX; i++) {
            Slime &slime = slimes[i];
            if (slime.id == slimeId) {
                assert(!"This slimeId is already in use!");
            }
        }
    }
#endif

    for (int i = 0; i < WORLD_ENTITIES_MAX; i++) {
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

    for (int i = 0; i < WORLD_ENTITIES_MAX; i++) {
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
        const Vector3 slimeBC = sprite_world_center(slime->sprite, slime->body.position, slime->sprite.scale);
        particleSystem.GenerateFX(ParticleFX_Goo, 20, slimeBC, 2.0);
        printf("Despawn slime %u\n", slimeId);
        memset(slime, 0, sizeof(*slime));
    }
}

void BloodParticlesFollowPlayer(ParticleFX &effect, void *userData)
{
    assert(effect.type == ParticleFX_Blood);
    assert(userData);

    Player *player = (Player *)userData;
    effect.origin = player->GetAttachPoint(Player::AttachPoint::Gut);
}

void World::Simulate(double dt)
{
    SimPlayers(dt);
    SimSlimes(dt);
}

void World::SimPlayers(double dt)
{
    for (size_t i = 0; i < SERVER_PLAYERS_MAX; i++) {
        Player &player = players[i];
        if (!player.id) {
            continue;
        }

        assert(map);
        player.Update(dt, *map);

        if (player.actionState == Player::ActionState::Attacking && player.combat.attackFrame == 1) {
            const float playerAttackReach = METERS_TO_PIXELS(1.0f);
            float playerDamage;

            const Item &selectedItem = player.GetSelectedItem();
            switch (selectedItem.type) {
                case ItemType::Weapon:
                {
                    const Item_Weapon &weapon = static_cast<const Item_Weapon &>(selectedItem);
                    playerDamage = weapon.damage;
                    break;
                }
                default:
                {
                    playerDamage = player.combat.meleeDamage;
                    break;
                }
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
                        //sound_catalog_play(SoundID::Squeak, 0.75f + dlb_rand32f_variance(0.2f));

                        uint32_t coins = lootSystem.RollCoins(slime.combat.lootTableId, (int)slime.sprite.scale);
                        assert(coins);

                        // TODO(design): Convert coins to higher currency if stack fills up?
                        player.inventory.slots[(int)PlayerInventorySlot::Coins].stackCount += coins;
                        player.stats.coinsCollected += coins;

                        Vector3 deadCenter = sprite_world_center(slime.sprite, slime.body.position, slime.sprite.scale);
                        particleSystem.GenerateFX(ParticleFX_Gold, (size_t)coins, deadCenter, 2.0);
                        sound_catalog_play(SoundID::Gold, 1.0f + dlb_rand32f_variance(0.1f));

                        DespawnSlime(slime.id);
                        player.stats.slimesSlain++;
                    } else {
                        SoundID squish = dlb_rand32i_range(0, 1) ? SoundID::Squish1 : SoundID::Squish2;
                        sound_catalog_play(squish, 1.0f + dlb_rand32f_variance(0.2f));
                    }
                    slimesHit++;
                }
            }

            if (slimesHit) {
                sound_catalog_play(SoundID::Slime_Stab1, 1.0f + dlb_rand32f_variance(0.1f));
            }
            sound_catalog_play(SoundID::Whoosh, 1.0f + dlb_rand32f_variance(0.1f));
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

    //static double lastSquish = 0;
    //double timeSinceLastSquish = now - lastSquish;

    Player *randomAlivePlayer = 0;
    for (size_t i = 0; i < SERVER_PLAYERS_MAX; i++) {
        if (players[i].id && players[i].combat.hitPoints > 0) {
            randomAlivePlayer = &players[i];
        }
    }

    for (size_t slimeIdx = 0; slimeIdx < WORLD_ENTITIES_MAX; slimeIdx++) {
        Slime &slime = slimes[slimeIdx];
        if (!slime.id) {
            continue;
        }

        // TODO: Actually find closest alive player via RTree
        Player *closestPlayer = randomAlivePlayer;
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
            const float moveRandMult = dlb_rand32i_range(1, 100) > 5 ? 1.0f : -1.0f;
            const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
            const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
            const Vector2 slimePos = slime.body.GroundPosition();
            const Vector2 slimePosNew = v2_add(slimePos, slimeMove);

            int willCollide = 0;
            for (size_t collideIdx = slimeIdx + 1; collideIdx < WORLD_ENTITIES_MAX; collideIdx++) {
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
                SoundID squish = dlb_rand32i_range(0, 1) ? SoundID::Squish1 : SoundID::Squish2;
                sound_catalog_play(squish, 1.0f + dlb_rand32f_variance(0.2f));
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
                    ParticleFX *bloodParticles = particleSystem.GenerateFX(ParticleFX_Blood, 32, playerGut, bleedDuration);
                    if (bloodParticles) {
                        bloodParticles->callbacks[(int)ParticleFXEvent_BeforeUpdate].function = BloodParticlesFollowPlayer;
                        bloodParticles->callbacks[(int)ParticleFXEvent_BeforeUpdate].userData = closestPlayer;
                    }
                    lastBleed = glfwGetTime();
                }
            }
        }

        slime.Update(dt);
    }
}

void World::GenerateSnapshot(WorldSnapshot &worldSnapshot)
{
    assert(worldSnapshot.playerId);
    assert(worldSnapshot.tick);

    assert(ARRAY_SIZE(worldSnapshot.players) <= ARRAY_SIZE(players));
    assert(ARRAY_SIZE(worldSnapshot.slimes)  <= ARRAY_SIZE(slimes));
    assert(ARRAY_SIZE(worldSnapshot.players) == WORLD_SNAPSHOT_PLAYERS_MAX);
    assert(ARRAY_SIZE(worldSnapshot.slimes)  == WORLD_SNAPSHOT_ENTITIES_MAX);

    Player *player = FindPlayer(worldSnapshot.playerId);
    if (!player) {
        return;
    }

    // TODO: Find players/slimes/etc. that are actually near the player this snapshot is being generated for
    for (size_t i = 0; i < SERVER_PLAYERS_MAX; i++) {
        memcpy(worldSnapshot.players[i].name, players[i].name, USERNAME_LENGTH_MAX);
        worldSnapshot.players[worldSnapshot.playerCount].id           = players[i].id                 ;
        worldSnapshot.players[worldSnapshot.playerCount].nameLength   = players[i].nameLength         ;
        worldSnapshot.players[worldSnapshot.playerCount].position     = players[i].body.position      ;
        worldSnapshot.players[worldSnapshot.playerCount].hitPoints    = players[i].combat.hitPoints   ;
        worldSnapshot.players[worldSnapshot.playerCount].hitPointsMax = players[i].combat.hitPointsMax;
        worldSnapshot.playerCount++;
        if (worldSnapshot.playerCount > WORLD_SNAPSHOT_PLAYERS_MAX) {
            break;
        }
    }
    for (size_t i = 0; i < WORLD_ENTITIES_MAX; i++) {
        if (v3_length_sq(v3_sub(player->body.position, slimes[i].body.position)) < SQUARED(1300.0f)) {
            worldSnapshot.slimes[worldSnapshot.slimeCount].id           = slimes[i].id                 ;
            worldSnapshot.slimes[worldSnapshot.slimeCount].position     = slimes[i].body.position      ;
            worldSnapshot.slimes[worldSnapshot.slimeCount].hitPoints    = slimes[i].combat.hitPoints   ;
            worldSnapshot.slimes[worldSnapshot.slimeCount].hitPointsMax = slimes[i].combat.hitPointsMax;
            worldSnapshot.slimes[worldSnapshot.slimeCount].scale        = slimes[i].sprite.scale       ;
            worldSnapshot.slimeCount++;
            if (worldSnapshot.slimeCount > WORLD_SNAPSHOT_ENTITIES_MAX) {
                break;
            }
        }
    }
}