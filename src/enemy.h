#pragma once
#include "body.h"
#include "combat.h"
#include "draw_command.h"
#include "sprite.h"

//enum class SlimeAction {
//    SlimeAction_None   = 0,
//    SlimeAction_Attack = 1,
//};

struct Enemy : public Drawable {
    enum Type : uint8_t {
        Type_Slime = 0,
        Type_Count
    };

    enum class MoveState {
        Idle = 0,
        Jump = 1
    };

    enum class ActionState {
        None      = 0,
        Attacking = 1,
    };

    uint32_t    id           {};
    Type        type         {};
    uint32_t    nameLength   {};
    char        name         [USERNAME_LENGTH_MAX]{};
    Body3D      body         {};
    Combat      combat       {};
    Sprite      sprite       {};
    MoveState   moveState    {};
    ActionState actionState  {};
    double      randJumpIdle {};

    void Init    (Type type);
    void SetName (const char *newName, uint32_t newNameLength);

    Vector3 WorldCenter    (void) const;
    Vector3 WorldTopCenter (void) const;

    void UpdateDirection (Vector2 offset);
    void Update          (World &world, double dt);

    float Depth (void) const;
    bool  Cull  (const Rectangle& cullRect) const;
    void  Draw  (World &world);
};