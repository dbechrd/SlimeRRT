#include "townfolk.h"

namespace Townfolk
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

        entity->SetName(CSTR("Townfolk"));
        //body3d->speed = SV_SLIME_MOVE_SPEED;
        //body3d->drag = 0.01f;
        //body3d->friction = 0.95f;
        //body3d->restitution = 0.5f;
        combat->flags |= Combat::Flag_TooBigToFail;
        //combat->level = 1;
        combat->hitPointsMax = 1;
        combat->hitPoints = combat->hitPointsMax;
        //combat->meleeDamage = 3.0f;
        //combat->xp = 3;
        // TODO: Shop inventory?
        //combat->lootTableId = LootTableID::LT_Slime;
        sprite->scale = 1;

        // TODO: Look this up by npc.type in Draw() instead
        if (!g_clock.server) {
            Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Monster_Slime);
            SpriteDef *spriteDef = spritesheet.FindSprite("blue_slime");
            sprite->spriteDef = spriteDef;
        }
        return ErrorType::Success;
    }

    void Update(World &world, EntityID entityId, double dt)
    {
        DLB_ASSERT(entityId);
        Body3D *townfolkBody3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
        Sprite *townfolkSprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
        DLB_ASSERT(townfolkBody3d);
        DLB_ASSERT(townfolkSprite);

        // Find nearest player
        Vector2 toNearestPlayer{};
        EntityID nearestPlayer = world.PlayerFindNearest(
            townfolkBody3d->GroundPosition(),
            10.0f,
            &toNearestPlayer,
            false
        );

        if (nearestPlayer) {
            // Turn toward nearest player
            sprite_set_direction(*townfolkSprite, toNearestPlayer);
        }
    }
}