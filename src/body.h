#pragma once
#include "direction.h"
#include "helpers.h"
#include "maths.h"
#include "ring_buffer.h"
#include "raylib/raylib.h"

struct Vector3Snapshot {
    double    recvAt    {};
    Vector3i  v         {};
    Direction direction {};
};

struct Body3D {
    Vector3i acceleration {};
    Vector3i velocity     {};
    Vector3i prevPosition {};
    Vector3i position     {};
    float    rotation     {};
    float    restitution  {};  // 0 = no bounce    1 = 100% bounce
    float    drag         {};  // 0 = no drag      1 = 100% drag
    float    friction     {};  // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)
    double   lastUpdated  {};
    double   lastMoved    {};
    bool     landed       {};
    bool     bounced      {};
    bool     idle         {};
    bool     idleChanged  {};

    RingBuffer<Vector3Snapshot, CL_WORLD_HISTORY> positionHistory {};

    Vector3i BottomCenter() const;
    Vector3i GroundPosition() const;
    bool OnGround() const;
    bool Resting() const;
    void Update(double dt);
};