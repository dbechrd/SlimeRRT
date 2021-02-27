#pragma once
#include "raylib.h"
#include <math.h>

static inline Vector2 v2_init(float x, float y)
{
    Vector2 result = { 0 };
    result.x = x;
    result.y = y;
    return result;
}

static inline int v2_is_zero(const Vector2 v)
{
    return v.x == 0.0f && v.y == 0.0f;
}

static inline Vector2 v2_add(const Vector2 a, const Vector2 b)
{
    Vector2 result = { 0 };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

static inline Vector2 v2_sub(const Vector2 a, const Vector2 b)
{
    Vector2 result = { 0 };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

static inline float v2_length_sq(const Vector2 v)
{
    const float lengthSq = v.x * v.x + v.y * v.y;
    return lengthSq;
}

static inline float v2_length(const Vector2 v)
{
    const float length = sqrtf(v2_length_sq(v));
    return length;
}

static inline Vector2 v2_normalize(const Vector2 v)
{
    if (v2_is_zero(v)) {
        return v;
    }

    const float length = v2_length(v);
    Vector2 result = { 0 };
    result.x = v.x / length;
    result.y = v.y / length;
    return result;
}

static inline float v2_dot(const Vector2 a, const Vector2 b)
{
    const float dot = a.x * b.x + a.y * b.y;
    return dot;
}

static inline float v2_distance_sq(const Vector2 a, const Vector2 b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float distance_sq = dx * dx + dy * dy;
    return distance_sq;
}

static inline float v2_distance(const Vector2 a, const Vector2 b)
{
    const float distance = sqrtf(v2_distance_sq(a, b));
    return distance;
}