#include "sim.h"
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
#include "dlb_rand.h"
#include <cassert>

void BloodParticlesFollowPlayer(ParticleEffect *effect, void *userData)
{
    assert(effect);
    assert(effect->type == ParticleEffectType_Blood);
    assert(userData);

    Player *player = (Player *)userData;
    effect->origin = player_get_attach_point(player, PlayerAttachPoint_Gut);
}

void sim(double now, double dt, const PlayerControllerState input, Player *player, Tilemap *map, Slime *slimes,
    const SpriteDef *coinSpriteDef)
{
    assert(player);
    assert(map);
    assert(slimes);

    if (input.selectSlot) {
        assert(input.selectSlot >= PlayerInventorySlot_1);
        assert(input.selectSlot <= PlayerInventorySlot_6);
        player->inventory.selectedSlot = input.selectSlot;
    }

    float playerSpeed = 4.0f;
    Vector2 moveBuffer = { 0 };
    if (input.moveState == PlayerMoveState_Running) {
        playerSpeed += 2.0f;
    }

    if (input.moveState) {
        switch (input.direction) {
            case Direction_North     : moveBuffer = { 0.0f,-1.0f }; break;
            case Direction_East      : moveBuffer = { 1.0f, 0.0f }; break;
            case Direction_South     : moveBuffer = { 0.0f, 1.0f }; break;
            case Direction_West      : moveBuffer = {-1.0f, 0.0f }; break;
            case Direction_NorthEast : moveBuffer = { 1.0f,-1.0f }; break;
            case Direction_SouthEast : moveBuffer = { 1.0f, 1.0f }; break;
            case Direction_SouthWest : moveBuffer = {-1.0f, 1.0f }; break;
            case Direction_NorthWest : moveBuffer = {-1.0f,-1.0f }; break;
            default: assert(!"Invalid direction");
        }
    }

    Vector2 moveOffset = v2_scale(v2_normalize(moveBuffer), METERS_TO_PIXELS(playerSpeed) * (float)dt);
    if (!v2_is_zero(moveOffset)) {
        const Vector2 curPos = body_ground_position(&player->body);
        const Tile *curTile = tilemap_at_world_try(map, (int)curPos.x, (int)curPos.y);
        const bool curWalkable = tile_is_walkable(curTile);

        Vector2 newPos = v2_add(curPos, moveOffset);
        Tile *newTile = tilemap_at_world_try(map, (int)newPos.x, (int)newPos.y);

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
                newTile = tilemap_at_world_try(map, (int)newPos.x, (int)newPos.y);
                if (tile_is_walkable(newTile)) {
                    // X offset is walkable
                    moveOffset.y = 0.0f;
                } else {
                    // X unwalkable, try only Y offset
                    newPos = curPos;
                    newPos.y += moveOffset.y;
                    newTile = tilemap_at_world_try(map, (int)newPos.x, (int)newPos.y);
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

        if (player_move(player, now, dt, moveOffset)) {
            static double lastFootstep = 0;
            double timeSinceLastFootstep = now - lastFootstep;
            if (timeSinceLastFootstep > 1.0f / playerSpeed) {
                sound_catalog_play(SoundID_Footstep, 1.0f + dlb_rand32f_variance(0.5f));
                lastFootstep = now;
            }
        }
    }

    if (input.actionState == PlayerActionState_Attacking) {
        const float playerAttackReach = METERS_TO_PIXELS(1.0f);

        if (player_attack(player, now, dt)) {
            float playerDamage;

            const Item *selectedItem = player_selected_item(player);
            switch (selectedItem->type) {
                case ItemType_Weapon: {
                    playerDamage = selectedItem->data.weapon.damage;
                    break;
                }
                default: {
                    playerDamage = player->combat.meleeDamage;
                    break;
                }
            }

            size_t slimesHit = 0;
            for (size_t slimeIdx = 0; slimeIdx < MAX_SLIMES; slimeIdx++) {
                Slime *slime = &slimes[slimeIdx];
                if (!slime->combat.hitPoints)
                    continue;

                Vector3 playerToSlime = v3_sub(slime->body.position, player->body.position);
                if (v3_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                    player->stats.damageDealt += MIN(slime->combat.hitPoints, playerDamage);
                    slime->combat.hitPoints = MAX(0.0f, slime->combat.hitPoints - playerDamage);
                    if (!slime->combat.hitPoints) {
                        //sound_catalog_play(SoundID_Squeak, 0.75f + dlb_rand32f_variance(0.2f));

                        int coins = dlb_rand32i_range(1, 4) * (int)slime->sprite.scale;
                        // TODO(design): Convert coins to higher currency if stack fills up?
                        player->inventory.slots[PlayerInventorySlot_Coins].stackCount += coins;
                        player->stats.coinsCollected += coins;

                        Vector3 deadCenter = sprite_world_center(&slime->sprite, slime->body.position, slime->sprite.scale);
                        particle_effect_create(ParticleEffectType_Goo, 20, deadCenter, 2.0, now, 0);
                        particle_effect_create(ParticleEffectType_Gold, (size_t)coins, deadCenter, 2.0, now, coinSpriteDef);
                        sound_catalog_play(SoundID_Gold, 1.0f + dlb_rand32f_variance(0.1f));

                        player->stats.slimesSlain++;
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

    player_update(player, now, dt);

    {
        // TODO: Move these to somewhere
        const float slimeMoveSpeed = METERS_TO_PIXELS(2.0f);
        const float slimeAttackReach = METERS_TO_PIXELS(0.5f);
        const float slimeAttackTrack = METERS_TO_PIXELS(10.0f);
        const float slimeAttackDamage = 1.0f;
        const float slimeRadius = METERS_TO_PIXELS(0.5f);

        //static double lastSquish = 0;
        //double timeSinceLastSquish = now - lastSquish;

        for (size_t slimeIdx = 0; slimeIdx < MAX_SLIMES; slimeIdx++) {
            Slime *slime = &slimes[slimeIdx];
            if (!slime->combat.hitPoints)
                continue;

            Vector2 slimeToPlayer = v2_sub(body_ground_position(&player->body), body_ground_position(&slime->body));
            const float slimeToPlayerDistSq = v2_length_sq(slimeToPlayer);
            if (slimeToPlayerDistSq > SQUARED(slimeAttackReach) &&
                slimeToPlayerDistSq <= SQUARED(slimeAttackTrack))
            {
                const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
                const float moveDist = MIN(slimeToPlayerDist, slimeMoveSpeed * slime->sprite.scale);
                // 25% -1.0, 75% +1.0f
                const float moveRandMult = dlb_rand32i_range(1, 4) > 1 ? 1.0f : -1.0f;
                const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
                const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
                const Vector2 slimePos = body_ground_position(&slime->body);
                const Vector2 slimePosNew = v2_add(slimePos, slimeMove);

                int willCollide = 0;
                for (size_t collideIdx = 0; collideIdx < MAX_SLIMES; collideIdx++) {
                    if (collideIdx == slimeIdx)
                        continue;

                    if (!slimes[collideIdx].combat.hitPoints)
                        continue;

                    Vector2 otherSlimePos = body_ground_position(&slimes[collideIdx].body);
                    const float zDist = fabsf(slime->body.position.z - slimes[collideIdx].body.position.z);
                    const float radiusScaled = slimeRadius * slime->sprite.scale;
                    if (v2_length_sq(v2_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                        Slime *a = &slimes[slimeIdx];
                        Slime *b = &slimes[collideIdx];
                        if (slime_combine(a, b)) {
                            const Slime *dead = a->combat.hitPoints == 0.0f ? a : b;
                            const Vector3 slimeBC = sprite_world_center(
                                &dead->sprite, dead->body.position, dead->sprite.scale
                            );
                            particle_effect_create(ParticleEffectType_Goo, 20, slimeBC, 2.0, now, 0);
                        }
                    }
                    if (v2_length_sq(v2_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                        willCollide = 1;
                    }
                }

                if (!willCollide && slime_move(&slimes[slimeIdx], now, dt, slimeMove)) {
                    SoundID squish = dlb_rand32i_range(0, 1) ? SoundID_Squish1 : SoundID_Squish2;
                    //sound_catalog_play(squish, 1.0f + dlb_rand32f_variance(0.2f));
                }
            }

            // Allow slime to attack if on the ground and close enough to the player
            slimeToPlayer = v2_sub(body_ground_position(&player->body), body_ground_position(&slime->body));
            if (v2_length_sq(slimeToPlayer) <= SQUARED(slimeAttackReach)) {
                if (slime_attack(&slimes[slimeIdx], now, dt)) {
                    player->combat.hitPoints = MAX(
                        0.0f,
                        player->combat.hitPoints - (slimeAttackDamage * slime->sprite.scale)
                    );

                    static double lastBleed = 0;
                    const double bleedDuration = 3.0;

                    if (now - lastBleed > bleedDuration / 3.0) {
                        Vector3 playerGut = player_get_attach_point(player, PlayerAttachPoint_Gut);
                        ParticleEffect *bloodParticles = particle_effect_create(
                            ParticleEffectType_Blood, 32, playerGut, bleedDuration, now, 0
                        );
                        if (bloodParticles) {
                            bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].function = BloodParticlesFollowPlayer;
                            bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].userData = player;
                        }
                        lastBleed = now;
                    }
                }
            }

            slime_update(&slimes[slimeIdx], now, dt);
        }
    }

    particles_update(now, dt);
}