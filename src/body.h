#pragma once
#include "spritesheet.h"
#include "raylib.h"

typedef enum Facing {
    Facing_Idle,
    Facing_West,
    Facing_East,
    Facing_South,
    Facing_North,
    Facing_NorthEast,
    Facing_NorthWest,
    Facing_SouthWest,
    Facing_SouthEast,
    Facing_Count
} Facing;

typedef struct Body3D {
    Vector3 acceleration;
    Vector3 velocity;
    Vector3 position;
    float scale;
    float restitution;  // 0 = no bounce    1 = 100% bounce
    float drag;         // 0 = no drag      1 = 100% drag
    float friction;     // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)
    double lastMoveTime;
    Facing facing;
    float alpha;        // transparency of sprite
    Sprite *sprite;
    size_t spriteFrameIdx;
} Body3D;

SpriteFrame *body_frame      (const Body3D *body);
Rectangle body_frame_rect    (const Body3D *body);
Rectangle body_rect          (const Body3D *body);
Vector3 body_center          (const Body3D *body);
Vector2 body_ground_position (const Body3D *body);
void body_update             (Body3D *body, float dt);
void body_draw               (const Body3D *body);