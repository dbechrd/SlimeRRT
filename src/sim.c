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
#include "spritesheet_catalog.h"
#include "tilemap.h"
#include "dlb_rand.h"
#include <assert.h>

static void GoldParticlesStarted(ParticleEffect *effect, void *userData)
{
    UNUSED(userData);
    static const Sprite *coinSprite = 0;
    if (!coinSprite) {
        const Spritesheet *coinSpritesheet = spritesheet_catalog_find(SpritesheetID_Coin);
        coinSprite = spritesheet_find_sprite(coinSpritesheet, "coin");
    }

    for (size_t i = 0; i < effect->particleCount; i++) {
        effect->particles[i].body.sprite = coinSprite;
    }
    sound_catalog_play(SoundID_Gold, 1.0f + dlb_rand_variance(0.1f));
}

void sim(double now, double dt, const PlayerControllerState input, Player *player, Tilemap *map, Slime *slimes,
    size_t slimeCount)
{
    assert(player);
    assert(map);
    assert(!slimeCount || slimes);

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
            case Direction_North     : moveBuffer = (Vector2) { 0.0f,-1.0f }; break;
            case Direction_East      : moveBuffer = (Vector2) { 1.0f, 0.0f }; break;
            case Direction_South     : moveBuffer = (Vector2) { 0.0f, 1.0f }; break;
            case Direction_West      : moveBuffer = (Vector2) {-1.0f, 0.0f }; break;
            case Direction_NorthEast : moveBuffer = (Vector2) { 1.0f,-1.0f }; break;
            case Direction_SouthEast : moveBuffer = (Vector2) { 1.0f, 1.0f }; break;
            case Direction_SouthWest : moveBuffer = (Vector2) {-1.0f, 1.0f }; break;
            case Direction_NorthWest : moveBuffer = (Vector2) {-1.0f,-1.0f }; break;
            default: assert(!"Invalid direction");
        }
    }

    Vector2 moveOffset = v2_round(v2_scale(v2_normalize(moveBuffer), playerSpeed));
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
            if (timeSinceLastFootstep > 1.0 / (double)playerSpeed) {
                sound_catalog_play(SoundID_Footstep, 1.0f + dlb_rand_variance(0.5f));
                lastFootstep = now;
            }
        }
    }

    if (input.actionState == PlayerActionState_Attacking) {
        const float playerAttackReach = METERS(1.0f);

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
            for (size_t slimeIdx = 0; slimeIdx < slimeCount; slimeIdx++) {
                if (!slimes[slimeIdx].combat.hitPoints)
                    continue;

                Vector3 playerToSlime = v3_sub(slimes[slimeIdx].body.position, player->body.position);
                if (v3_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                    slimes[slimeIdx].combat.hitPoints = MAX(0.0f, slimes[slimeIdx].combat.hitPoints - playerDamage);
                    if (!slimes[slimeIdx].combat.hitPoints) {
                        //sound_catalog_play(SoundID_Squeak, 0.75f + dlb_rand_variance(0.2f));

                        int coins = dlb_rand_int(1, 4) * (int)slimes[slimeIdx].body.scale;
                        // TODO(design): Convert coins to higher currency if stack fills up?
                        player->inventory.slots[PlayerInventorySlot_Coins].stackCount += coins;
                        player->stats.coinsCollected += coins;

                        ParticleEffect *gooParticles = particle_effect_alloc(ParticleEffectType_Goo, 20);
                        //gooParticles->callbacks[ParticleEffectEvent_Started].function = GooParticlesStarted;
                        //gooParticles->callbacks[ParticleEffectEvent_Started].userData = gooSprite;
                        //gooParticles->callbacks[ParticleEffectEvent_Dying].function = GooParticlesDying;
                        Vector3 deadCenter = body_center(&slimes[slimeIdx].body);
                        particle_effect_start(gooParticles, now, 2.0, deadCenter);

                        ParticleEffect *goldParticles = particle_effect_alloc(ParticleEffectType_Gold, (size_t)coins);
                        goldParticles->callbacks[ParticleEffectEvent_Started].function = GoldParticlesStarted;
                        Vector3 slimeBC = body_center(&slimes[slimeIdx].body);
                        particle_effect_start(goldParticles, now, 2.0, slimeBC);

                        player->stats.slimesKilled++;
                    } else {
                        SoundID squish = dlb_rand_int(0, 1) ? SoundID_Squish1 : SoundID_Squish2;
                        sound_catalog_play(squish, 1.0f + dlb_rand_variance(0.2f));
                    }
                    slimesHit++;
                }
            }

            if (slimesHit) {
                sound_catalog_play(SoundID_Slime_Stab1, 1.0f + dlb_rand_variance(0.1f));
            }
            sound_catalog_play(SoundID_Whoosh, 1.0f + dlb_rand_variance(0.1f));
        }
    }
}