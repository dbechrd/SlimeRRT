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

    float   speed        {};  // move speed, in meters
    Vector3 velocity     {};
    float   rotation     {};
    float   restitution  {};  // 0 = no bounce    1 = 100% bounce
    float   drag         {};  // 0 = no drag      1 = 100% drag
    float   friction     {};  // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)
    float   gravityScale {};  // 1 = normal gravity

    Body3D(void);
    Vector3 WorldPosition(void) const;
    Vector3 WorldPositionServer(void) const;
    Vector2 GroundPosition(void) const;
    Vector2 GroundPositionServer(void) const;
    Vector2 PrevGroundPosition(void) const;
    Vector2 VisualPosition(void) const;
    void Teleport(Vector3 pos);
    void Move(Vector2 offset);
    void Move3D(Vector3 offset);
    bool OnGround(void) const;
    bool Resting(void) const;
    bool Idle(void) const;
    bool Jumped(void) const;
    bool Landed(void) const;
    bool Bounced(void) const;
    double TimeSinceLastMove(void) const;
    void ApplyForce(Vector3 force);
    void Update(double dt);

private:
    const char *LOG_SRC = "Body";
    Vector3 positionPrev {};
    Vector3 destPosition {};  // buffer all non-sim move offsets (teleport, etc.)
    Vector3 position     {};
    double  lastUpdated  {};
    double  lastMoved    {};
    bool    jumped       {};
    bool    landed       {};
    bool    bounced      {};
    bool    idle         {};
    bool    idleChanged  {};
};