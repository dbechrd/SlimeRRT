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

void BloodParticlesFollowPlayer(ParticleEffect &effect, void *userData)
{
    assert(effect.type == ParticleEffectType_Blood);
    assert(userData);

    Player *player = (Player *)userData;
    effect.origin = player->GetAttachPoint(Player::AttachPoint_Gut);
}

void World::Sim(double now, double dt, const PlayerControllerState input, World &world, const SpriteDef *coinSpriteDef)
{
    for (int i = 0; i < PlayerInventorySlot_Count; i++) {
        if (input.selectSlot[i]) {
            world.player->m_inventory.selectedSlot = (PlayerInventorySlot)i;
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
            world.player->m_moveState = Player::MoveState_Running;
            playerSpeed += 2.0f;
        } else {
            world.player->m_moveState = Player::MoveState_Walking;
        }
    } else {
        world.player->m_moveState = Player::MoveState_Idle;
    }

    Vector2 moveOffset = v2_scale(v2_normalize(moveBuffer), METERS_TO_PIXELS(playerSpeed) * (float)dt);
    if (!v2_is_zero(moveOffset)) {
        const Vector2 curPos = body_ground_position(&world.player->m_body);
        const Tile *curTile = tilemap_at_world_try(world.map, (int)curPos.x, (int)curPos.y);
        const bool curWalkable = tile_is_walkable(curTile);

        Vector2 newPos = v2_add(curPos, moveOffset);
        Tile *newTile = tilemap_at_world_try(world.map, (int)newPos.x, (int)newPos.y);

        // NOTE: This extra logic allows the player to slide when attempting to move diagonally against a wall
        // NOTE: If current tile isn't walkable, allow player to walk off it. This may not be the best solution
        // if the player can accidentally end up on unwalkable tiles through gameplay, but currently the only
        // way to end up on an unwalkable tile is to spawn there.
        // TODO: We should fix spawning to ensure player spawns on walkable tile (can probably just manually
        // generate something interesting in the center of the world that overwrites procgen, like Don't
        // Starve's fancy arrival portal.
        if (curWalkable) {
            if (!tile_is_walkable(newTile)) {
                // XY unwalkable, try only X offset
                newPos = curPos;
                newPos.x += moveOffset.x;
                newTile = tilemap_at_world_try(world.map, (int)newPos.x, (int)newPos.y);
                if (tile_is_walkable(newTile)) {
                    // X offset is walkable
                    moveOffset.y = 0.0f;
                } else {
                    // X unwalkable, try only Y offset
                    newPos = curPos;
                    newPos.y += moveOffset.y;
                    newTile = tilemap_at_world_try(world.map, (int)newPos.x, (int)newPos.y);
                    if (tile_is_walkable(newTile)) {
                        // Y offset is walkable
                        moveOffset.x = 0.0f;
                    } else {
                        // XY, and both slide directions are all unwalkable
                        moveOffset.x = 0.0f;
                        moveOffset.y = 0.0f;

                        // TODO: Play wall bonk sound (or splash for water? heh)
                        // TODO: Maybe bounce the player against the wall? This code doesn't do that nicely..
                        //player_move(&charlie, v2_scale(v2_negate(moveOffset), 10.0f));
                    }
                }
            }
        }

        if (world.player->Move(now, dt, moveOffset)) {
            static double lastFootstep = 0;
            double timeSinceLastFootstep = now - lastFootstep;
            if (timeSinceLastFootstep > 1.0f / playerSpeed) {
                sound_catalog_play(SoundID_Footstep, 1.0f + dlb_rand32f_variance(0.5f));
                lastFootstep = now;
            }
        }
    }

    if (input.attack) {
        const float playerAttackReach = METERS_TO_PIXELS(1.0f);

        if (world.player->Attack(now, dt)) {
            float playerDamage;

            const Item &selectedItem = world.player->GetSelectedItem();
            switch (selectedItem.type) {
                case ItemType_Weapon: {
                    playerDamage = selectedItem.data.weapon.damage;
                    break;
                }
                default: {
                    playerDamage = world.player->m_combat.meleeDamage;
                    break;
                }
            }

            size_t slimesHit = 0;
            for (Slime &slime : world.slimes) {
                if (!slime.m_combat.hitPoints)
                    continue;

                Vector3 playerToSlime = v3_sub(slime.m_body.position, world.player->m_body.position);
                if (v3_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                    world.player->m_stats.damageDealt += MIN(slime.m_combat.hitPoints, playerDamage);
                    slime.m_combat.hitPoints = MAX(0.0f, slime.m_combat.hitPoints - playerDamage);
                    if (!slime.m_combat.hitPoints) {
                        //sound_catalog_play(SoundID_Squeak, 0.75f + dlb_rand32f_variance(0.2f));

                        int coins = dlb_rand32i_range(1, 4) * (int)slime.m_sprite.scale;
                        // TODO(design): Convert coins to higher currency if stack fills up?
                        world.player->m_inventory.slots[PlayerInventorySlot_Coins].stackCount += coins;
                        world.player->m_stats.coinsCollected += coins;

                        Vector3 deadCenter = sprite_world_center(slime.m_sprite, slime.m_body.position, slime.m_sprite.scale);
                        particle_effect_create(ParticleEffectType_Goo, 20, deadCenter, 2.0, now, 0);
                        particle_effect_create(ParticleEffectType_Gold, (size_t)coins, deadCenter, 2.0, now, coinSpriteDef);
                        sound_catalog_play(SoundID_Gold, 1.0f + dlb_rand32f_variance(0.1f));

                        world.player->m_stats.slimesSlain++;
                    } else {
                        SoundID squish = dlb_rand32i_range(0, 1) ? SoundID_Squish1 : SoundID_Squish2;
                        sound_catalog_play(squish, 1.0f + dlb_rand32f_variance(0.2f));
                    }
                    slimesHit++;
                }
            }

            if (slimesHit) {
                sound_catalog_play(SoundID_Slime_Stab1, 1.0f + dlb_rand32f_variance(0.1f));
            }
            sound_catalog_play(SoundID_Whoosh, 1.0f + dlb_rand32f_variance(0.1f));
        }
    }

    world.player->Update(now, dt);

    {
        // TODO: Move these to somewhere
        const float slimeMoveSpeed = METERS_TO_PIXELS(2.0f);
        const float slimeAttackReach = METERS_TO_PIXELS(0.5f);
        const float slimeAttackTrack = METERS_TO_PIXELS(10.0f);
        const float slimeAttackDamage = 1.0f;
        const float slimeRadius = METERS_TO_PIXELS(0.5f);

        //static double lastSquish = 0;
        //double timeSinceLastSquish = now - lastSquish;

        size_t slimeCount = world.slimes.size();
        for (size_t slimeIdx = 0; slimeIdx < slimeCount; slimeIdx++) {
            Slime &slime = world.slimes[slimeIdx];
            if (!slime.m_combat.hitPoints)
                continue;

            Vector2 slimeToPlayer = v2_sub(body_ground_position(&world.player->m_body), body_ground_position(&slime.m_body));
            const float slimeToPlayerDistSq = v2_length_sq(slimeToPlayer);
            if (slimeToPlayerDistSq > SQUARED(slimeAttackReach) &&
                slimeToPlayerDistSq <= SQUARED(slimeAttackTrack))
            {
                const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
                const float moveDist = MIN(slimeToPlayerDist, slimeMoveSpeed * slime.m_sprite.scale);
                // 25% -1.0, 75% +1.0f
                const float moveRandMult = dlb_rand32i_range(1, 4) > 1 ? 1.0f : -1.0f;
                const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
                const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
                const Vector2 slimePos = body_ground_position(&slime.m_body);
                const Vector2 slimePosNew = v2_add(slimePos, slimeMove);

                int willCollide = 0;
                for (size_t collideIdx = slimeIdx + 1; collideIdx < slimeCount; collideIdx++) {
                    Slime &otherSlime = world.slimes[collideIdx];
                    if (!otherSlime.m_combat.hitPoints)
                        continue;

                    Vector2 otherSlimePos = body_ground_position(&otherSlime.m_body);
                    const float zDist = fabsf(slime.m_body.position.z - otherSlime.m_body.position.z);
                    const float radiusScaled = slimeRadius * slime.m_sprite.scale;
                    if (v2_length_sq(v2_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                        if (slime.Combine(otherSlime)) {
                            const Slime &dead = slime.m_combat.hitPoints == 0.0f ? slime : otherSlime;
                            const Vector3 slimeBC = sprite_world_center(dead.m_sprite, dead.m_body.position, dead.m_sprite.scale
                            );
                            particle_effect_create(ParticleEffectType_Goo, 20, slimeBC, 2.0, now, 0);
                        }
                    }
                    if (v2_length_sq(v2_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                        willCollide = 1;
                    }
                }

                if (!willCollide && slime.Move(now, dt, slimeMove)) {
                    SoundID squish = dlb_rand32i_range(0, 1) ? SoundID_Squish1 : SoundID_Squish2;
                    sound_catalog_play(squish, 1.0f + dlb_rand32f_variance(0.2f));
                }
            }

            // Allow slime to attack if on the ground and close enough to the player
            slimeToPlayer = v2_sub(body_ground_position(&world.player->m_body), body_ground_position(&slime.m_body));
            if (v2_length_sq(slimeToPlayer) <= SQUARED(slimeAttackReach)) {
                if (slime.Attack(now, dt)) {
                    world.player->m_combat.hitPoints = MAX(
                        0.0f,
                        world.player->m_combat.hitPoints - (slimeAttackDamage * slime.m_sprite.scale)
                    );

                    static double lastBleed = 0;
                    const double bleedDuration = 3.0;

                    if (now - lastBleed > bleedDuration / 3.0) {
                        Vector3 playerGut = world.player->GetAttachPoint(Player::AttachPoint_Gut);
                        ParticleEffect *bloodParticles = particle_effect_create(
                            ParticleEffectType_Blood, 32, playerGut, bleedDuration, now, 0
                        );
                        if (bloodParticles) {
                            bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].function = BloodParticlesFollowPlayer;
                            bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].userData = world.player;
                        }
                        lastBleed = now;
                    }
                }
            }

            slime.Update(now, dt);
        }
    }

    particles_update(now, dt);
}