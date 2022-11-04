#pragma once
#include "body.h"
#include "combat.h"
#include "draw_command.h"
#include "sprite.h"

enum FacetType {
    Facet_Entity,
    Facet_Body,
    Facet_Combat,
    Facet_Sprite,
    Facet_Inventory,
    Facet_Count
};

typedef uint32_t EntityID;

struct Facet {
    EntityID  entityId;
    FacetType facetType;
};

// These determine the behavior of the facets, i.e. which systems
// are responsible for updating them.
enum EntityType {
    Entity_Player   = 0,
    Entity_Slime    = 1,
    Entity_Townfolk = 2,
    Entity_Count
};

struct Entity : public Facet {
    EntityType  entityType   {};
    uint32_t    nameLength   {};
    char        name         [ENTITY_NAME_LENGTH_MAX]{};

    // TODO: Move to Body3D or Pathfind or AI or something..
    enum MoveState {
        Move_Idle = 0,
        Move_Walk = 1,
        Move_Run  = 2,
        Move_Jump = 3,
    };
    // TODO: Move to Combat or Brain or something..
    enum ActionState {
        Act_None    = 0,
        Act_Attack  = 1,
        // attack recovery.. we could also have hit recovery.. is that different?
        Act_Recover = 2,
    };
    MoveState   moveState    {};
    ActionState actionState  {};
    double      despawnedAt  {};

    void    SetName         (const char *newName, uint32_t newNameLength);
    Vector3 WorldCenter     (void) const;
    Vector3 WorldTopCenter  (void) const;
    float   TakeDamage      (float damage);
    void    UpdateDirection (Vector2 offset);
    void    Update          (World &world, double dt);
    float   Depth           (void) const;
    bool    Cull            (const Rectangle& cullRect) const;
    void    Draw            (World &world, Vector2 at) const override;
};
