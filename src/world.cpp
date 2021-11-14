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

void World::InitPlayer(Player &player, const char *name, size_t nameLength)
{
    if (player.combat.maxHitPoints != 0) {
        return;
    }

    const Spritesheet &charlieSpritesheet = SpritesheetCatalog::spritesheets[(int)SpritesheetID::Charlie];
    const SpriteDef *charlieSpriteDef = charlieSpritesheet.FindSprite("player_sword");

    player.nameLength = MIN(nameLength, USERNAME_LENGTH_MAX);
    memcpy(player.name, name, player.nameLength);
    player.body.position = GetWorldSpawn();
    player.combat.maxHitPoints = 100.0f;
    player.combat.hitPoints = player.combat.maxHitPoints;
    player.sprite.scale = 1.0f;
    player.sprite.spriteDef = charlieSpriteDef;
}

Player *World::SpawnPlayer(const char *name, size_t nameLength)
{
    for (int i = 0; i < SERVER_MAX_PLAYERS; i++) {
        Player &player = players[i];
        if (player.combat.maxHitPoints == 0) {
            playerCount++;
            InitPlayer(player, name, nameLength);
            return &player;
        }
    }
    return 0;
}

void World::SyncPlayers(const Player *players, size_t playersLength)
{
    assert(playersLength <= SERVER_MAX_PLAYERS);

    playerCount = playersLength;
    for (size_t i = 0; i < playersLength; i++) {
        Player &player = this->players[i];
        InitPlayer(player, players[i].name, players[i].nameLength);
        player.body.position.x = players[i].body.position.x;
        player.body.position.y = players[i].body.position.y;
    }
}

void World::SyncEntities(const Slime *entities, size_t entityLength)
{
    assert(entityLength <= WORLD_ENTITIES_MAX);
    const Spritesheet &slimeSpritesheet = SpritesheetCatalog::spritesheets[(int)SpritesheetID::Slime];
    const SpriteDef *slimeSpriteDef = slimeSpritesheet.FindSprite("slime");

    slimeCount = entityLength;
    for (size_t i = 0; i < entityLength; i++) {
        Slime &slime = slimes[i];
        slime.combat.maxHitPoints = 10.0f;
        slime.combat.hitPoints = slime.combat.maxHitPoints;
        slime.combat.meleeDamage = 0.0f;
        slime.combat.lootTableId = LootTableID::LT_Slime;
        slime.body.position.x = entities[i].body.position.x;
        slime.body.position.y = entities[i].body.position.y;
        slime.sprite.scale = 1.0f;
        slime.sprite.spriteDef = slimeSpriteDef;
    }
}

void BloodParticlesFollowPlayer(ParticleFX &effect, void *userData)
{
    assert(effect.type == ParticleFX_Blood);
    assert(userData);

    Player *player = (Player *)userData;
    effect.origin = player->GetAttachPoint(Player::AttachPoint::Gut);
}

void World::Sim(double now, double dt, const PlayerControllerState input)
{
    assert(players[0].combat.maxHitPoints);

    for (int i = 0; i < (int)PlayerInventorySlot::Count; i++) {
        if (input.selectSlot[i]) {
            players[0].inventory.selectedSlot = (PlayerInventorySlot)i;
            break;
        }
    }

    float playerSpeed = 4.0f;
    Vector2 moveBuffer = {};

    if (input.walkNorth || input.walkEast || input.walkSouth || input.walkWest) {
        if (input.walkNorth) {
            moveBuffer.y -= 1.0f;
        }
        if (input.walkEast) {
            moveBuffer.x += 1.0f;
        }
        if (input.walkSouth) {
            moveBuffer.y += 1.0f;
        }
        if (input.walkWest) {
            moveBuffer.x -= 1.0f;
        }
        if (input.run) {
            players[0].moveState = Player::MoveState::Running;
            playerSpeed += 2.0f;
        } else {
            players[0].moveState = Player::MoveState::Walking;
        }
    } else {
        players[0].moveState = Player::MoveState::Idle;
    }

    Vector2 moveOffset = v2_scale(v2_normalize(moveBuffer), METERS_TO_PIXELS(playerSpeed) * (float)dt);
    if (!v2_is_zero(moveOffset)) {
        const Vector2 curPos = players[0].body.GroundPosition();
        const Tile *curTile = map->TileAtWorldTry(curPos.x, curPos.y, 0, 0);
        const bool curWalkable = curTile && curTile->IsWalkable();

        Vector2 newPos = v2_add(curPos, moveOffset);
        Tile *newTile = map->TileAtWorldTry(newPos.x, newPos.y, 0, 0);

        // NOTE: This extra logic allows the player to slide when attempting to move diagonally against a wall
        // NOTE: If current tile isn't walkable, allow player to walk off it. This may not be the best solution
        // if the player can accidentally end up on unwalkable tiles through gameplay, but currently the only
        // way to end up on an unwalkable tile is to spawn there.
        // TODO: We should fix spawning to ensure player spawns on walkable tile (can probably just manually
        // generate something interesting in the center of the world that overwrites procgen, like Don't
        // Starve's fancy arrival portal.
        if (curWalkable) {
            if (!newTile || !newTile->IsWalkable()) {
                // XY unwalkable, try only X offset
                newPos = curPos;
                newPos.x += moveOffset.x;
                newTile = map->TileAtWorldTry(newPos.x, newPos.y, 0, 0);
                if (!newTile || !newTile->IsWalkable()) {
                    // X unwalkable, try only Y offset
                    newPos = curPos;
                    newPos.y += moveOffset.y;
                    newTile = map->TileAtWorldTry(newPos.x, newPos.y, 0, 0);
                    if (!newTile || !newTile->IsWalkable()) {
                        // XY, and both slide directions are all unwalkable
                        moveOffset.x = 0.0f;
                        moveOffset.y = 0.0f;

                        // TODO: Play wall bonk sound (or splash for water? heh)
                        // TODO: Maybe bounce the player against the wall? This code doesn't do that nicely..
                        //player_move(&charlie, v2_scale(v2_negate(moveOffset), 10.0f));
                    } else {
                        // Y offset is walkable
                        moveOffset.x = 0.0f;
                    }
                } else {
                    // X offset is walkable
                    moveOffset.y = 0.0f;
                }
            }
        }

        if (players[0].Move(now, dt, moveOffset)) {
            static double lastFootstep = 0;
            double timeSinceLastFootstep = now - lastFootstep;
            if (timeSinceLastFootstep > 1.0f / playerSpeed) {
                sound_catalog_play(SoundID::Footstep, 1.0f + dlb_rand32f_variance(0.5f));
                lastFootstep = now;
            }
        }

        if (players[0].body.position.x < 0.0f) {
            assert(!"wtf?");
        }
    }

    if (input.attack) {
        const float playerAttackReach = METERS_TO_PIXELS(1.0f);

        if (players[0].Attack(now, dt)) {
            float playerDamage;

            const Item &selectedItem = players[0].GetSelectedItem();
            switch (selectedItem.type) {
                case ItemType::Weapon: {
                    const Item_Weapon &weapon = static_cast<const Item_Weapon &>(selectedItem);
                    playerDamage = weapon.damage;
                    break;
                }
                default: {
                    playerDamage = players[0].combat.meleeDamage;
                    break;
                }
            }

            size_t slimesHit = 0;
            for (Slime &slime : slimes) {
                if (!slime.combat.hitPoints)
                    continue;

                Vector3 playerToSlime = v3_sub(slime.body.position, players[0].body.position);
                if (v3_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                    players[0].stats.damageDealt += MIN(slime.combat.hitPoints, playerDamage);
                    slime.combat.hitPoints = MAX(0.0f, slime.combat.hitPoints - playerDamage);
                    if (!slime.combat.hitPoints) {
                        //sound_catalog_play(SoundID::Squeak, 0.75f + dlb_rand32f_variance(0.2f));

                        uint32_t coins = loot_table_roll_coins(slime.combat.lootTableId, (int)slime.sprite.scale);
                        assert(coins);

                        // TODO(design): Convert coins to higher currency if stack fills up?
                        players[0].inventory.slots[(int)PlayerInventorySlot::Coins].stackCount += coins;
                        players[0].stats.coinsCollected += coins;

                        Vector3 deadCenter = sprite_world_center(slime.sprite, slime.body.position, slime.sprite.scale);
                        particleSystem.GenerateFX(ParticleFX_Goo, 20, deadCenter, 2.0, now);
                        particleSystem.GenerateFX(ParticleFX_Gold, (size_t)coins, deadCenter, 2.0, now);
                        sound_catalog_play(SoundID::Gold, 1.0f + dlb_rand32f_variance(0.1f));

                        players[0].stats.slimesSlain++;
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

    players[0].Update(now, dt);

    //SimSlimes(now, dt);

    particleSystem.Update(now, dt);
}

void World::SimSlimes(double now, double dt)
{
    // TODO: Move these to somewhere
    const float slimeMoveSpeed = METERS_TO_PIXELS(2.0f);
    const float slimeAttackReach = METERS_TO_PIXELS(0.5f);
    const float slimeAttackTrack = METERS_TO_PIXELS(10.0f);
    const float slimeRadius = METERS_TO_PIXELS(0.5f);

    //static double lastSquish = 0;
    //double timeSinceLastSquish = now - lastSquish;

    for (size_t slimeIdx = 0; slimeIdx < slimeCount; slimeIdx++) {
        Slime &slime = slimes[slimeIdx];
        if (!slime.combat.hitPoints)
            continue;

        // TODO: Actually find closest player via RTree
        Player *closestPlayer = &players[0];
        if (!closestPlayer)
            continue;

        Vector2 slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), slime.body.GroundPosition());
        const float slimeToPlayerDistSq = v2_length_sq(slimeToPlayer);
        if (slimeToPlayerDistSq <= SQUARED(slimeAttackTrack)) {
            const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
            const float moveDist = MIN(slimeToPlayerDist, slimeMoveSpeed * slime.sprite.scale);
            const float moveRandMult = dlb_rand32f_range(-0.25f, 0.95f);
            const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
            const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
            const Vector2 slimePos = slime.body.GroundPosition();
            const Vector2 slimePosNew = v2_add(slimePos, slimeMove);

            int willCollide = 0;
            for (size_t collideIdx = slimeIdx + 1; collideIdx < slimeCount; collideIdx++) {
                Slime &otherSlime = slimes[collideIdx];
                if (!otherSlime.combat.hitPoints)
                    continue;

                Vector2 otherSlimePos = otherSlime.body.GroundPosition();
                const float zDist = fabsf(slime.body.position.z - otherSlime.body.position.z);
                const float radiusScaled = slimeRadius * slime.sprite.scale;
                if (v2_length_sq(v2_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                    if (slime.Combine(otherSlime)) {
                        const Slime &dead = slime.combat.hitPoints == 0.0f ? slime : otherSlime;
                        const Vector3 slimeBC = sprite_world_center(dead.sprite, dead.body.position, dead.sprite.scale
                        );
                        particleSystem.GenerateFX(ParticleFX_Goo, 20, slimeBC, 2.0, now);
                    }
                }
                if (v2_length_sq(v2_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                    willCollide = 1;
                }
            }

            if (!willCollide && slime.Move(now, dt, slimeMove)) {
                SoundID squish = dlb_rand32i_range(0, 1) ? SoundID::Squish1 : SoundID::Squish2;
                sound_catalog_play(squish, 1.0f + dlb_rand32f_variance(0.2f));
            }
        }

        // Allow slime to attack if on the ground and close enough to the player
        slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), slime.body.GroundPosition());
        if (v2_length_sq(slimeToPlayer) <= SQUARED(slimeAttackReach)) {
            if (slime.Attack(now, dt)) {
                closestPlayer->combat.hitPoints = MAX(
                    0.0f,
                    closestPlayer->combat.hitPoints - (slime.combat.meleeDamage * slime.sprite.scale)
                );

                static double lastBleed = 0;
                const double bleedDuration = 1.0;

                if (now - lastBleed > bleedDuration / 3.0) {
                    Vector3 playerGut = closestPlayer->GetAttachPoint(Player::AttachPoint::Gut);
                    ParticleFX *bloodParticles = particleSystem.GenerateFX(
                        ParticleFX_Blood, 32, playerGut, bleedDuration, now
                    );
                    if (bloodParticles) {
                        bloodParticles->callbacks[(int)ParticleFXEvent_BeforeUpdate].function = BloodParticlesFollowPlayer;
                        bloodParticles->callbacks[(int)ParticleFXEvent_BeforeUpdate].userData = closestPlayer;
                    }
                    lastBleed = now;
                }
            }
        }

        slime.Update(now, dt);
    }
}