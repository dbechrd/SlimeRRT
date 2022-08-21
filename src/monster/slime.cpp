#include "../enemy.h"
#include "../catalog/spritesheets.h"

namespace Monster
{
    void slime_init(Enemy &slime)
    {
        slime.Init(Enemy::Type_Slime);
        slime.SetName(CSTR("Slime"));

        slime.body.speed = SV_SLIME_MOVE_SPEED;
        slime.body.drag = 0.95f;
        slime.body.friction = 0.95f;

        slime.combat.level = 1;
        slime.combat.hitPointsMax = 10.0f;
        slime.combat.hitPoints = slime.combat.hitPointsMax;
        slime.combat.meleeDamage = 10.0f;
        slime.combat.xpMin = 2;
        slime.combat.xpMax = 4;
        slime.combat.lootTableId = LootTableID::LT_Slime;
        //enemy.combat.droppedHitLoot = false;
        //enemy.combat.droppedDeathLoot = false;

        slime.sprite.scale = 1.0f;
        if (!g_clock.server) {
            const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Slime);
            const SpriteDef *spriteDef = spritesheet.FindSprite("slime");
            slime.sprite.spriteDef = spriteDef;
        }
    }


    static bool slime_move(Enemy &slime, double dt, Vector2 offset)
    {
        UNUSED(dt);  // todo: use dt

        // Slime must be on ground to move
        if (!slime.body.OnGround()) {
            return false;
        }

        slime.moveState = Enemy::MoveState::Idle;
        if (v2_is_zero(offset)) {
            return false;
        }

        // Check if hasn't moved for a bit
        if (slime.body.TimeSinceLastMove() > slime.randJumpIdle) {
            slime.moveState = Enemy::MoveState::Jump;
            slime.body.velocity.x += offset.x;
            slime.body.velocity.y += offset.y;
            slime.body.velocity.z += METERS_TO_PIXELS(4.0f);
            slime.randJumpIdle = (double)dlb_rand32f_range(0.5f, 1.5f) / slime.sprite.scale;
            slime.UpdateDirection(offset);
            return true;
        }
        return false;
    }

    static bool slime_try_combine(Enemy &slime, Enemy &other)
    {
        assert(other.id > slime.id);

        // The bigger slime should absorb the smaller one
        Enemy *a = nullptr;
        Enemy *b = nullptr;
        if (slime.sprite.scale > other.sprite.scale) {
            a = &slime;
            b = &other;
        } else {
            a = &other;
            b = &slime;
        }

        // Limit max scale
        float newScale = a->sprite.scale + 0.5f * b->sprite.scale;
        if (newScale > SLIME_MAX_SCALE) {
            return false;
        }

        // Combine slime B's attributes into slime A
        a->sprite.scale = newScale;
        a->combat.hitPoints = a->combat.hitPoints + 0.5f * b->combat.hitPoints;
        a->combat.hitPointsMax = a->combat.hitPointsMax + 0.5f * b->combat.hitPointsMax;
        //Vector3 halfAToB = v3_scale(v3_sub(b->body.position, a->body.position), 0.5f);
        //a->body.position = v3_add(a->body.position, halfAToB);

        // Kill slime B
        b->combat.hitPoints = 0.0f;
        b->combat.diedAt = g_clock.now;
        b->combat.droppedDeathLoot = true;
#if SV_DEBUG_WORLD_ENEMIES
        TraceLog(LOG_DEBUG, "Combined slime #%u into slime #%u", b->id, a->id);
#endif
        return true;
    }

    bool slime_attack(Enemy &slime, double dt)
    {
        UNUSED(dt); // todo: use dt;

#if 0
        if (!body.landed) {
            return false;
        }

        const double timeSinceAttackStarted = now - combat.attackStartedAt;
        if (timeSinceAttackStarted > (combat.attackDuration / sprite.scale)) {
            action = Action::Attack;
            combat.attackStartedAt = now;
            combat.attackDuration = 0.1;
            return true;
        }
#else
        if (slime.body.JustLanded()) {
            slime.actionState = Enemy::ActionState::Attacking;
            slime.body.Move({});  // update last move to stop idle animation
            slime.combat.attackStartedAt = g_clock.now;
            slime.combat.attackDuration = 0.0;
            return true;
        }
#endif
        return false;
    }

    void slime_update(Enemy &slime, double dt, World &world)
    {
        assert(slime.type == Enemy::Type_Slime);

        // Find nearest player
        float enemyToPlayerDistSq = 0.0f;
        Player *closestPlayer = world.FindClosestPlayer(slime.body.GroundPosition(), SV_ENEMY_DESPAWN_RADIUS, &enemyToPlayerDistSq);
        if (!closestPlayer) {
            // No nearby players, insta-kill enemy w/ no loot
            slime.combat.DealDamage(slime.combat.hitPoints);
            slime.combat.droppedDeathLoot = true;
            return;
        }

        // Allow enemy to move toward nearest player
        if (enemyToPlayerDistSq <= SQUARED(SV_SLIME_ATTACK_TRACK)) {
            Vector2 slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), slime.body.GroundPosition());
            const float slimeToPlayerDist = sqrtf(enemyToPlayerDistSq);
            const float moveDist = MIN(slimeToPlayerDist, METERS_TO_PIXELS(slime.body.speed) * slime.sprite.scale);
            // 5% -1.0, 95% +1.0f
            const float moveRandMult = 1.0f; //dlb_rand32i_range(1, 100) > 5 ? 1.0f : -1.0f;
            const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
            const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
            const Vector3 slimePos = slime.body.WorldPosition();
            const Vector3 slimePosNew = v3_add(slimePos, { slimeMove.x, slimeMove.y, 0 });

            int willCollide = 0;
            for (size_t collideIdx = 0; collideIdx < SV_MAX_ENEMIES; collideIdx++) {
                Enemy &otherSlime = world.enemies[collideIdx];
                if (!otherSlime.id || otherSlime.combat.diedAt || otherSlime.id <= slime.id) {
                    continue;
                }

                Vector3 otherSlimePos = otherSlime.body.WorldPosition();
                const float radiusScaled = SV_SLIME_RADIUS * slime.sprite.scale;
                if (v3_length_sq(v3_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled)) {
                    slime_try_combine(slime, otherSlime);
                }
                if (v3_length_sq(v3_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled)) {
                    willCollide = 1;
                }
            }

            if (!willCollide && slime_move(slime, dt, slimeMove)) {
                // TODO(cleanup): used to play sound effect, might do something else on server?
            }
        }

        // Allow slime to attack if on the ground and close enough to the player
        if (enemyToPlayerDistSq <= SQUARED(SV_SLIME_ATTACK_REACH)) {
            if (!world.peaceful && slime_attack(slime, dt)) {
                closestPlayer->combat.DealDamage(slime.combat.meleeDamage * slime.sprite.scale);
            }
        }

        switch (slime.actionState) {
            case Enemy::ActionState::Attacking:
            {
                const double timeSinceAttackStarted = g_clock.now - slime.combat.attackStartedAt;
                if (timeSinceAttackStarted > slime.combat.attackDuration) {
                    slime.actionState = Enemy::ActionState::None;
                    slime.combat.attackStartedAt = 0;
                    slime.combat.attackDuration = 0;
                }
                break;
            }
        }
    }
}