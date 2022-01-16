#pragma once
#include "direction.h"
#include "helpers.h"
#include "ring_buffer.h"
#include "raylib/raylib.h"

struct Vector3Snapshot {
    double    recvAt    {};
    Vector3   v         {};
    Direction direction {};
};

struct Body3D {
    RingBuffer<Vector3Snapshot, CL_WORLD_HISTORY> positionHistory {};

    Vector3 velocity     {};
    float   rotation     {};
    float   restitution  {};  // 0 = no bounce    1 = 100% bounce
    float   drag         {};  // 0 = no drag      1 = 100% drag
    float   friction     {};  // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)

    Vector3 WorldPosition() const;
    Vector2 GroundPosition() const;
    Vector2 VisualPosition() const;
    void Teleport(const Vector3 &pos);
    void Move(const Vector2 &offset);
    bool Bounced() const;
    bool OnGround() const;
    bool JustLanded() const;
    bool Resting() const;
    bool Idle() const;
    double TimeSinceLastMove() const;
    void ApplyForce(const Vector3 &force);
    void Update(double dt);

private:
    Vector3 prevPosition {};
    Vector3 position     {};
    double  lastUpdated  {};
    double  lastMoved    {};
    bool    landed       {};
    bool    bounced      {};
    bool    idle         {};
    bool    idleChanged  {};
};