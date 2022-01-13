#pragma once
#include "body.h"
#include "combat.h"
#include "draw_command.h"
#include "sprite.h"

//enum class SlimeAction {
//    SlimeAction_None   = 0,
//    SlimeAction_Attack = 1,
//};

struct Slime : public Drawable {
    enum class MoveState {
        Idle = 0,
        Jump = 1
    };

    enum class ActionState {
        None      = 0,
        Attacking = 1,
    };

    uint32_t    id           {};
    uint32_t    nameLength   {};
    char        name         [USERNAME_LENGTH_MAX]{};
    Body3D      body         {};
    Combat      combat       {};
    Sprite      sprite       {};
    MoveState   moveState    {};
    ActionState actionState  {};
    double      randJumpIdle {};

    void Init   (void);
    void SetName(const char *name, uint32_t nameLength);

    bool Move(double dt, Vector2 offset);
    bool Combine(Slime &other);
    bool Attack(double dt);
    void Update(double dt);

    float Depth(void) const;
    bool  Cull(const Rectangle& cullRect) const;
    void  Draw(void) const;

private:
    void UpdateDirection(Vector2 offset);
};