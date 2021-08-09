#pragma once
#include "body.h"
#include "combat.h"
#include "sprite.h"

//enum SlimeAction {
//    SlimeAction_None   = 0,
//    SlimeAction_Attack = 1,
//};

struct Slime {
public:
    enum Action {
        Action_None   = 0,
        Action_Attack = 1,
    };

public:
    Slime(const char *name, const SpriteDef &spriteDef);

public:
    bool Move(double now, double dt, Vector2 offset);
    bool Combine(Slime &other);
    bool Attack(double now, double dt);
    void Update(double now, double dt);
    float Depth() const;
    bool Cull(const Rectangle &cullRect) const;
    void Push() const;
    void Draw() const;

private:
    Slime() = default;
    void UpdateDirection(Vector2 offset);

public:
    const char *name;
    Body3D body;
    Combat combat;
    Sprite sprite;
    Action action;
    double randJumpIdle;
};