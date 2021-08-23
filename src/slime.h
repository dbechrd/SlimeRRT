#pragma once
#include "body.h"
#include "combat.h"
#include "drawable.h"
#include "sprite.h"

//enum class SlimeAction {
//    SlimeAction_None   = 0,
//    SlimeAction_Attack = 1,
//};

struct Slime : public Drawable {
    enum class Action {
        None   = 0,
        Attack = 1,
    };

    const char *name         {};
    Body3D      body         {};
    Combat      combat       {};
    Action      action       {};
    double      randJumpIdle {};

    Slime(const char *slimeName, const SpriteDef &spriteDef);

    float Depth() const override;
    bool Cull(const Rectangle &cullRect) const override;
    void Draw() const override;

    bool Move(double now, double dt, Vector2 offset);
    bool Combine(Slime &other);
    bool Attack(double now, double dt);
    void Update(double now, double dt);

private:
    Slime() = default;
    void UpdateDirection(Vector2 offset);
};