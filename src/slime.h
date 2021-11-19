#pragma once
#include "body.h"
#include "combat.h"
#include "draw_command.h"
#include "sprite.h"

//enum class SlimeAction {
//    SlimeAction_None   = 0,
//    SlimeAction_Attack = 1,
//};

struct Slime {
    enum class Action {
        None   = 0,
        Jump   = 1,
        Attack = 2,
    };

    uint32_t    id           {};
    uint32_t    nameLength   {};
    char        name         [USERNAME_LENGTH_MAX]{};
    Body3D      body         {};
    Combat      combat       {};
    Sprite      sprite       {};
    Action      action       {};
    double      randJumpIdle {};

    void Init   (void);
    void SetName(const char *name, uint32_t nameLength);

    bool Move(double now, double dt, Vector2 offset);
    bool Combine(Slime &other);
    bool Attack(double now, double dt);
    void Update(double now, double dt);
    void Push(DrawList &drawList) const;

private:
    void UpdateDirection(Vector2 offset);
};

float Slime_Depth (const Drawable &drawable);
bool  Slime_Cull  (const Drawable &drawable, const Rectangle &cullRect);
void  Slime_Draw  (const Drawable &drawable);