#pragma once
#include "raylib.h"

typedef struct Transform2D {
    Vector2 acceleration;
    Vector2 velocity;
    Vector2 position;
} Transform2D;

void transform_update(Transform2D *transform, float dt);
