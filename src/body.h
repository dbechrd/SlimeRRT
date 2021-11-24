#pragma once
#include "helpers.h"
#include "ring_buffer.h"
#include "raylib/raylib.h"

struct Vector3Snapshot {
    double recvAt {};
    Vector3 v     {};
};

struct Body3D {
    Vector3 acceleration {};
    Vector3 velocity     {};
    Vector3 prevPosition {};
    Vector3 position     {};
    float   rotation     {};
    float   restitution  {};  // 0 = no bounce    1 = 100% bounce
    float   drag         {};  // 0 = no drag      1 = 100% drag
    float   friction     {};  // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)
    double  lastUpdated  {};
    double  lastMoved    {};
    bool    landed       {};
    bool    idle         {};

    RingBuffer<Vector3Snapshot, CL_WORLD_HISTORY> positionHistory {};

    Vector2 BottomCenter() const;
    Vector2 GroundPosition() const;
    bool OnGround() const;
    bool Resting() const;
    void Update(double dt);
};