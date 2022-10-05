#pragma once
#include "body.h"
#include "combat.h"
#include "draw_command.h"
#include "sprite.h"

class NPC : public Drawable {
public:
    enum Type {
        Type_None     = 0,
        Type_Slime    = 1,
        Type_Townfolk = 2,
        Type_Count
    };

    enum MoveState {
        Move_Idle = 0,
        Move_Jump = 1,
    };

    enum ActionState {
        Act_None   = 0,
        Act_Attack = 1,
    };

    uint32_t    id           {};
    Type        type         {};
    double      despawnedAt  {};
    uint32_t    nameLength   {};
    char        name         [ENTITY_NAME_LENGTH_MAX]{};
    Body3D      body         {};
    Combat      combat       {};
    Sprite      sprite       {};
    MoveState   moveState    {};
    ActionState actionState  {};

    union {
        struct {
            double randJumpIdle{};
        } slime;
    } state{};

    void    SetName         (const char *newName, uint32_t newNameLength);
    Vector3 WorldCenter     (void) const;
    Vector3 WorldTopCenter  (void) const;
    float   TakeDamage      (float damage);
    void    UpdateDirection (Vector2 offset);
    void    Update          (World &world, double dt);
    float   Depth           (void) const override;
    bool    Cull            (const Rectangle& cullRect) const override;
    void    Draw            (World &world) override;
};
