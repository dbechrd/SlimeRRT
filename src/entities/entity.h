#pragma once
#include "attach.h"
#include "draw_command.h"
#include "facet.h"
#include "../helpers.h"

// These determine the behavior of the facets, i.e. which systems
// are responsible for updating them.
enum EntityType {
    Entity_Player   = 0,
    Entity_Slime    = 1,
    Entity_Townfolk = 2,
    Entity_Count
};

// TODO: Move to Body3D or Pathfind or AI or something..
enum MoveState {
    Move_Idle = 0,
    Move_Walk = 1,
    Move_Run = 2,
    Move_Jump = 3,
};

// TODO: Move to Combat or Brain or something..
enum ActionState {
    Action_None = 0,
    Action_Attack = 1,
    // attack recovery.. we could also have hit recovery.. is that different?
    Action_Recover = 2,
};

struct Entity : public Facet, public Drawable {
    EntityType  entityType   {};
    uint32_t    nameLength   {};
    char        name         [ENTITY_NAME_LENGTH_MAX]{};

    MoveState   moveState    {};
    ActionState actionState  {};
    double      despawnedAt  {};

    union {
        struct {
            double randJumpIdle{};
        } slime;
    } state;

    static Vector3 WorldCenter      (World &world, EntityID entityId);
    static Vector3 WorldTopCenter3D (World &world, EntityID entityId);
    static Vector2 WorldTopCenter2D (World &world, EntityID entityId);
    static Vector3 GetAttachPoint   (World &world, EntityID entityId, AttachType attachType);
    static void    SlimeUpdate      (World &world, EntityID entityId, double dt);
    static void    Update           (World &world, EntityID entityId, double dt);
    static float   Depth            (World &world, EntityID entityId);
    static bool    Cull             (World &world, EntityID entityId, const Rectangle& cullRect);
    static void    DrawSwimOverlay  (World &world, EntityID entityId);

    void           Init             (World &world);
    void           SetName          (const char *newName, uint32_t newNameLength);
    void           Draw             (World &world, Vector2 at) override;

private:
    const char *LOG_SRC = "Entity";
};
