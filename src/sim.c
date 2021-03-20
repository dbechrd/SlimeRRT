#include "sim.h"
#include "body.h"
#include "helpers.h"
#include "maths.h"
#include "player.h"
#include "player_inventory.h"
#include "tilemap.h"

void sim(double now, double dt, Player *player, Tilemap *map)
{
    float playerSpeed = 4.0f;
    Vector2 moveBuffer = { 0 };
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        playerSpeed += 2.0f;
    }
    if (IsKeyDown(KEY_A)) moveBuffer.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveBuffer.x += 1.0f;
    if (IsKeyDown(KEY_W)) moveBuffer.y -= 1.0f;
    if (IsKeyDown(KEY_S)) moveBuffer.y += 1.0f;

    Vector2 moveOffset = v2_round(v2_scale(v2_normalize(moveBuffer), playerSpeed));
    if (!v2_is_zero(moveOffset)) {
        Vector2 curPos = body_ground_position(&player->body);
        Tile *curTile = tilemap_at_world_try(map, (int)curPos.x, (int)curPos.y);
        int curWalkable = tile_is_walkable(curTile);

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
                //SetSoundPitch(snd_footstep, 1.0f + dlb_rand_variance(0.5f));
                //PlaySound(snd_footstep);
                // TODO: SoundEffectPlay(SoundEffectType_PlayerFootstep);
                lastFootstep = now;
            }
        }
    }
}