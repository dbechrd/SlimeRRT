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

    const char *name         {};
    Body3D      body         {};
    Combat      combat       {};
    Sprite      sprite       {};
    Action      action       {};
    double      randJumpIdle {};

    Slime(const char *slimeName, const SpriteDef *spriteDef);

    bool Move(double now, double dt, Vector2 offset);
    bool Combine(Slime &other);
    bool Attack(double now, double dt);
    void Update(double now, double dt);
    void Push(DrawList &drawList) const;

private:
    Slime() = default;
    void UpdateDirection(Vector2 offset);
};

float Slime_Depth (const Drawable &drawable);
bool  Slime_Cull  (const Drawable &drawable, const Rectangle &cullRect);
void  Slime_Draw  (const Drawable &drawable);