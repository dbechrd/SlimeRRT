#include "slime.h"
#include "../catalog/spritesheets.h"

void Slime::Init(NPC &npc)
{
    npc.type = NPC::Type_Slime;
    npc.SetName(CSTR("Slime"));
    npc.body.speed = SV_SLIME_MOVE_SPEED;
    npc.body.drag = 0.95f;
    npc.body.friction = 0.95f;
    npc.combat.level = 1;
    npc.combat.hitPointsMax = 10.0f;
    npc.combat.hitPoints = npc.combat.hitPointsMax;
    npc.combat.meleeDamage = 10.0f;
    npc.combat.xpMin = 2;
    npc.combat.xpMax = 4;
    npc.combat.lootTableId = LootTableID::LT_Slime;
    //npc.combat.droppedHitLoot = false;
    //npc.combat.droppedDeathLoot = false;
    npc.sprite.scale = 1.0f;

    // TODO: Look this up by npc.type in Draw() instead
    if (!g_clock.server) {
        const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Slime);
        const SpriteDef *spriteDef = spritesheet.FindSprite("slime");
        npc.sprite.spriteDef = spriteDef;
    }
}

bool Slime::Move(NPC &npc, double dt, Vector2 offset)
{
    UNUSED(dt);  // todo: use dt

    // Slime must be on ground to move
    if (!npc.body.OnGround()) {
        return false;
    }

    npc.moveState = NPC::Move_Idle;
    if (v2_is_zero(offset)) {
        return false;
    }

    // Check if hasn't moved for a bit
    if (npc.body.TimeSinceLastMove() > npc.state.slime.randJumpIdle) {
        npc.moveState = NPC::Move_Jump;
        npc.body.ApplyForce({ offset.x, offset.y, METERS_TO_PIXELS(4.0f) });
        npc.state.slime.randJumpIdle = (double)dlb_rand32f_range(0.5f, 1.5f) / npc.sprite.scale;
        npc.UpdateDirection(offset);
        return true;
    }
    return false;
}

bool Slime::TryCombine(NPC &npc, NPC &other)
{
    return false;

    if (npc.type != NPC::Type_Slime || other.type != NPC::Type_Slime) {
        DLB_ASSERT("you can only combine slimes");
    }

    assert(other.id > npc.id);

    // The bigger slime should absorb the smaller one
    NPC *a = nullptr;
    NPC *b = nullptr;
    if (npc.sprite.scale > other.sprite.scale) {
        a = &npc;
        b = &other;
    } else {
        a = &other;
        b = &npc;
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
    E_DEBUG("Combined slime #%u into slime #%u", b->id, a->id);
#endif
    return true;
}

bool Slime::Attack(NPC &npc, double dt)
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
    if (npc.body.Landed()) {
        npc.actionState = NPC::Act_Attack;
        npc.body.Move({});  // update last move to stop idle animation
        npc.combat.attackStartedAt = g_clock.now;
        npc.combat.attackDuration = 0.0;
        return true;
    }
#endif
    return false;
}

void Slime::Update(NPC &npc, World &world, double dt)
{
    // Find nearest player
    float enemyToPlayerDistSq = 0.0f;
    Player *closestPlayer = world.FindClosestPlayer(
        npc.body.GroundPosition(), SV_ENEMY_DESPAWN_RADIUS, &enemyToPlayerDistSq
    );
    if (!closestPlayer) {
        // No nearby players, insta-kill enemy w/ no loot
        world.RemoveNpc(npc.id);
        //npc.combat.TakeDamage(npc.combat.hitPoints);
        //npc.combat.droppedDeathLoot = true;
        return;
    }

    // Allow enemy to move toward nearest player
    if (enemyToPlayerDistSq <= SQUARED(SV_SLIME_ATTACK_TRACK)) {
        Vector2 slimeToPlayer = v2_sub(closestPlayer->body.GroundPosition(), npc.body.GroundPosition());
        const float slimeToPlayerDist = sqrtf(enemyToPlayerDistSq);
        const float moveDist = MIN(slimeToPlayerDist, METERS_TO_PIXELS(npc.body.speed) * npc.sprite.scale);
        // 5% -1.0, 95% +1.0f
        const float moveRandMult = 1.0f; //dlb_rand32i_range(1, 100) > 5 ? 1.0f : -1.0f;
        const Vector2 slimeMoveDir = v2_normalize(slimeToPlayer);
        const Vector2 slimeMoveMag = v2_scale(slimeMoveDir, moveDist * moveRandMult);
        const Vector3 slimePos = npc.body.WorldPosition();
        const Vector3 slimePosNew = v3_add(slimePos, { slimeMoveMag.x, slimeMoveMag.y, 0 });

        int willCollide = 0;
        for (size_t collideIdx = 0; collideIdx < SV_MAX_NPCS; collideIdx++) {
            NPC &other = world.npcs[collideIdx];
            if (other.type != NPC::Type_Slime || other.combat.diedAt || other.id <= npc.id) {
                continue;
            }

            Vector3 otherSlimePos = other.body.WorldPosition();
            const float radiusScaled = SV_SLIME_RADIUS * npc.sprite.scale;
            if (v3_length_sq(v3_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled)) {
                TryCombine(npc, other);
            }
            if (v3_length_sq(v3_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled)) {
                willCollide = 1;
            }
        }

        if (!willCollide && Move(npc, dt, slimeMoveMag)) {
            // TODO(cleanup): used to play sound effect, might do something else on server?
            //if (g_clock.server) {
            //    Vector2 gPos = npc.body.GroundPosition();
            //    E_DEBUG("Updated slime %u @ %.f, %.f", npc.id, gPos.x, gPos.y);
            //}
        }
    }

    // Allow slime to attack if on the ground and close enough to the player
    if (enemyToPlayerDistSq <= SQUARED(SV_SLIME_ATTACK_REACH)) {
        if (!world.peaceful && Attack(npc, dt)) {
            closestPlayer->combat.TakeDamage(npc.combat.meleeDamage * npc.sprite.scale);
        }
    }

    switch (npc.actionState) {
        case NPC::Act_Attack:
        {
            const double timeSinceAttackStarted = g_clock.now - npc.combat.attackStartedAt;
            if (timeSinceAttackStarted > npc.combat.attackDuration) {
                npc.actionState = NPC::Act_None;
                npc.combat.attackStartedAt = 0;
                npc.combat.attackDuration = 0;
            }
            break;
        }
    }

    if (npc.body.Jumped()) {
        //Catalog::SoundID squish = dlb_rand32i_range(0, 1) ? Catalog::SoundID::Squish1 : Catalog::SoundID::Squish2;
        Catalog::SoundID squish = Catalog::SoundID::Squish1;
        Catalog::g_sounds.Play(squish, 1.0f + dlb_rand32f_variance(0.2f), true);
    }
}
