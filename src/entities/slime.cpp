#include "slime.h"
#include "../catalog/spritesheets.h"

namespace Slime
{
    ErrorType Init(World &world, EntityID entityId)
    {
        DLB_ASSERT(entityId);
        Entity *entity = world.facetDepot.EntityFind(entityId);
        DLB_ASSERT(entity);

        Body3D *body3d = 0;
        Combat *combat = 0;
        Sprite *sprite = 0;
        E_ERROR_RETURN(world.facetDepot.FacetAlloc(entityId, Facet_Body3D, (Facet **)&body3d), "Failed to alloc facet");
        E_ERROR_RETURN(world.facetDepot.FacetAlloc(entityId, Facet_Combat, (Facet **)&combat), "Failed to alloc facet");
        E_ERROR_RETURN(world.facetDepot.FacetAlloc(entityId, Facet_Sprite, (Facet **)&sprite), "Failed to alloc facet");

        entity->SetName(CSTR("Slime"));
        body3d->speed = SV_SLIME_MOVE_SPEED;
        body3d->drag = 0.01f;
        body3d->friction = 0.95f;
        body3d->restitution = 0.5f;
        combat->level = 1;
        combat->hitPointsMax = 10.0f;
        combat->hitPoints = combat->hitPointsMax;
        combat->meleeDamage = 3.0f;
        combat->xp = 3;
        combat->lootTableId = LootTableID::LT_Slime;
        sprite->scale = 1.0f;

        if (!g_clock.server) {
            Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Monster_Slime);
            SpriteDef *spriteDef = spritesheet.FindSprite("green_slime");
            sprite->spriteDef = spriteDef;
        }
        return ErrorType::Success;
    }

    bool TryCombine(World &world, EntityID slimeId, EntityID otherSlimeId)
    {
        DLB_ASSERT(slimeId);
        DLB_ASSERT(otherSlimeId);

        // Don't try to combine a slime with itself, very bad things will occur
        DLB_ASSERT(slimeId != otherSlimeId);
        if (slimeId == otherSlimeId) {
            return false;
        }

        Combat *combatA = (Combat *)world.facetDepot.FacetFind(slimeId, Facet_Combat);
        Entity *entityA = (Entity *)world.facetDepot.FacetFind(slimeId, Facet_Entity);
        Sprite *spriteA = (Sprite *)world.facetDepot.FacetFind(slimeId, Facet_Sprite);
        DLB_ASSERT(combatA);
        DLB_ASSERT(entityA);
        DLB_ASSERT(spriteA);

        Combat *combatB = (Combat *)world.facetDepot.FacetFind(otherSlimeId, Facet_Combat);
        Entity *entityB = (Entity *)world.facetDepot.FacetFind(otherSlimeId, Facet_Entity);
        Sprite *spriteB = (Sprite *)world.facetDepot.FacetFind(otherSlimeId, Facet_Sprite);
        DLB_ASSERT(combatB);
        DLB_ASSERT(entityB);
        DLB_ASSERT(spriteB);

        if (entityA->entityType != Entity_Slime || entityB->entityType != Entity_Slime) {
            E_WARN("you can only combine slimes", 0);
            DLB_ASSERT(!"ur stupid, stop that");
            return false;
        }

        // The bigger slime should absorb the smaller one
        if (spriteB->scale > spriteA->scale) {
            std::swap(combatA, combatB);
            std::swap(entityA, entityB);
            std::swap(spriteA, spriteB);
        }

        // Limit max scale
        float newScale = spriteA->scale + 0.5f * spriteB->scale;
        if (newScale > SLIME_MAX_SCALE) {
            return false;
        }

        // Combine slime B's attributes into slime A
        spriteA->scale = newScale;
        combatA->hitPoints = combatA->hitPoints + 0.5f * combatB->hitPoints;
        combatA->hitPointsMax = combatA->hitPointsMax + 0.5f * combatB->hitPointsMax;

        // Kill slime B
        entityB->despawnedAt = g_clock.now;
        //entityB->hitPoints = 0.0f;
        //entityB->diedAt = g_clock.now;
        //entityB->droppedDeathLoot = true;
    #if SV_DEBUG_WORLD_NPCS
        E_DEBUG("Combined slime #%u into slime #%u", b->id, a->id);
    #endif
        return true;
    }

    bool Move(World &world, EntityID entityId, double dt, Vector2 offset)
    {
        UNUSED(dt);  // todo: use dt

        DLB_ASSERT(entityId);
        Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
        Entity *entity = (Entity *)world.facetDepot.FacetFind(entityId, Facet_Entity);
        Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
        DLB_ASSERT(body3d);
        DLB_ASSERT(entity);
        DLB_ASSERT(sprite);

        // Slime must be on ground to move
        if (!body3d->OnGround()) {
            return false;
        }

        entity->moveState = Move_Idle;
        if (v2_is_zero(offset)) {
            return false;
        }

        // Check if hasn't moved for a bit
        if (body3d->TimeSinceLastMove() > entity->state.slime.randJumpIdle) {
            entity->moveState = Move_Jump;
            body3d->ApplyForce({ offset.x, offset.y, METERS_TO_PIXELS(5.0f) });
            // TODO: This is essentially Move_Recovery in a way.. hmm..
            entity->state.slime.randJumpIdle = (double)dlb_rand32f_range(0.5f, 1.5f) / sprite->scale;
            sprite_set_direction(*sprite, offset);
            return true;
        }
        return false;
    }

    bool Attack(World &world, EntityID entityId, double dt)
    {
        UNUSED(dt); // todo: use dt;

        DLB_ASSERT(entityId);
        Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
        Combat *combat = (Combat *)world.facetDepot.FacetFind(entityId, Facet_Combat);
        Entity *entity = (Entity *)world.facetDepot.FacetFind(entityId, Facet_Entity);
        DLB_ASSERT(body3d);
        DLB_ASSERT(combat);
        DLB_ASSERT(entity);

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
        if (body3d->landed) {
            entity->actionState = Action_Attack;
            body3d->Move({});  // update last move to stop idle animation
            combat->attackStartedAt = g_clock.now;
            combat->attackDuration = 0.0;
            return true;
        }
    #endif
        return false;
    }

    void Update(World &world, EntityID entityId, double dt)
    {
        DLB_ASSERT(entityId);
        Body3D *slimeBody3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
        Combat *slimeCombat = (Combat *)world.facetDepot.FacetFind(entityId, Facet_Combat);
        Entity *slimeEntity = world.facetDepot.EntityFind(entityId);
        Sprite *slimeSprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
        DLB_ASSERT(slimeBody3d);
        DLB_ASSERT(slimeCombat);
        DLB_ASSERT(slimeEntity);
        DLB_ASSERT(slimeSprite);

        // Find nearest player
        Vector2 toNearestPlayer{};
        EntityID nearestPlayer = world.PlayerFindNearest(
            slimeBody3d->GroundPosition(),
            SV_ENEMY_DESPAWN_RADIUS,
            &toNearestPlayer,
            false
        );

        // TODO: Make this more general for all enemies that should go away when nobody is nearby
        // Alternatively, we could store enemies in the world chunk if we want some sort of
        // mob continuity when a player returns to a previously visited area? Seems pointless.
        if (!nearestPlayer) {
            E_DEBUG("No nearby players, mark slime for despawn %u", entityId);
            slimeEntity->despawnedAt = g_clock.now;
            return;
        }

        // Allow enemy to move toward nearest player
        const float distToNearestPlayer = v2_length(toNearestPlayer);
        if (distToNearestPlayer <= (float)SV_SLIME_ATTACK_TRACK) {
            const float moveDist = MIN(distToNearestPlayer, METERS_TO_PIXELS(slimeBody3d->speed) * slimeSprite->scale);
            // 5% -1.0, 95% +1.0f
            const float moveRandMult = 1.0f; //dlb_rand32i_range(1, 100) > 5 ? 1.0f : -1.0f;
            const Vector2 slimeMoveDir = v2_normalize(toNearestPlayer);
            const Vector2 slimeMoveMag = v2_scale(slimeMoveDir, moveDist * moveRandMult);
            const Vector3 slimePos = slimeBody3d->WorldPosition();
            const Vector3 slimePosNew = v3_add(slimePos, { slimeMoveMag.x, slimeMoveMag.y, 0 });

            int willCollide = 0;
            for (EntityID otherSlimeId : world.facetDepot.entityIdsByType[Entity_Slime]) {
                // Cut O(n^2) in half
                if (otherSlimeId <= entityId) {
                    continue;
                }

                // Ignore dead slimes
                Combat *otherSlimeCombat = (Combat *)world.facetDepot.FacetFind(otherSlimeId, Facet_Combat);
                DLB_ASSERT(otherSlimeCombat);
                if (otherSlimeCombat->diedAt) {
                    continue;
                }

                Body3D *otherSlimeBody3d = (Body3D *)world.facetDepot.FacetFind(otherSlimeId, Facet_Body3D);

                Vector3 otherSlimePos = otherSlimeBody3d->WorldPosition();
                const float radiusScaled = SV_SLIME_RADIUS * slimeSprite->scale;
                if (v3_length_sq(v3_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled)) {
                    TryCombine(world, entityId, otherSlimeId);
                }
                if (v3_length_sq(v3_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled)) {
                    willCollide = 1;
                }
            }

            if (!willCollide && Move(world, entityId, dt, slimeMoveMag)) {
                // TODO(cleanup): used to play sound effect, might do something else on server?
                //if (g_clock.server) {
                //    Vector2 gPos = slime.body.GroundPosition();
                //    E_DEBUG("Updated slime %u @ %.f, %.f", slime.id, gPos.x, gPos.y);
                //}
            }
        }

        // Allow slime to attack if on the ground and close enough to the player
        if (distToNearestPlayer <= SV_SLIME_ATTACK_REACH) {
            if (!world.peaceful && Attack(world, entityId, dt)) {
                Combat *targetCombat = (Combat *)world.facetDepot.FacetFind(entityId, Facet_Combat);
                DLB_ASSERT(targetCombat);
                targetCombat->TakeDamage(slimeCombat->meleeDamage * slimeSprite->scale);
            }
        }

        switch (slimeEntity->actionState) {
            case Action_Attack: {
                const double timeSinceAttackStarted = g_clock.now - slimeCombat->attackStartedAt;
                if (timeSinceAttackStarted > slimeCombat->attackDuration) {
                    slimeEntity->actionState = Action_None;
                    slimeCombat->attackStartedAt = 0;
                    slimeCombat->attackDuration = 0;
                }
                break;
            }
        }
    }
}
