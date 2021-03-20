#pragma once
#include "helpers.h"
#include "raylib.h"

typedef struct Body3D {
    Vector3 acceleration;
    Vector3 velocity;
    Vector3 position;
    float rotation;
    float scale;
    float restitution;  // 0 = no bounce    1 = 100% bounce
    float drag;         // 0 = no drag      1 = 100% drag
    float friction;     // 0 = no friction  1 = 100% friction (when touching ground, i.e. z == 0.0f)
    double lastUpdated;
    double lastMoved;
    double lastAnimFrameStarted;
    bool landed;
    bool idle;
    Direction direction;
    Color color;
    const struct Sprite *sprite;
    size_t animFrameIdx;
} Body3D;

struct SpriteAnim *body_anim    (const Body3D *body);
struct SpriteFrame *body_frame  (const Body3D *body);
Rectangle body_frame_rect       (const Body3D *body);
Rectangle body_rect             (const Body3D *body);
Vector3 body_center             (const Body3D *body);
Vector2 body_ground_position    (const Body3D *body);
void body_update                (Body3D *body, double now, double dt);
void body_draw                  (const Body3D *body);