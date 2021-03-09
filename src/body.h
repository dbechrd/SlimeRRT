#pragma once
#include "raylib.h"

typedef struct Body3D {
    Vector3 acceleration;
    Vector3 velocity;
    Vector3 position;
    float scale;
    float restitution;  // 0 = no bounce    1 = 100% bounce
    float drag;         // 0 = no drag      1 = 100% drag
    float friction;     // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)
} Body3D;

void body3d_update(Body3D *body, float dt);
