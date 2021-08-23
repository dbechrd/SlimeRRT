#pragma once
#include "helpers.h"
#include "raylib.h"

struct Body3D {
    Vector3 acceleration {};
    Vector3 velocity     {};
    Vector3 position     {};
    float rotation       {};
    float restitution    {};  // 0 = no bounce    1 = 100% bounce
    float drag           {};  // 0 = no drag      1 = 100% drag
    float friction       {};  // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)
    double lastUpdated   {};
    double lastMoved     {};
    bool landed          {};
    bool idle            {};

    Vector2 BottomCenter() const;
    Vector2 GroundPosition() const;
    void Update(double now, double dt);
};