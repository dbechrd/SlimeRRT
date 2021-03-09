#pragma once
#include "raylib.h"
#include <math.h>

/// Vector2 ////////////////////////////////////////////////////////////////////

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

static inline int v2_is_tiny(const Vector2 v, float epsilon)
{
    int tiny = fabsf(v.x) < epsilon && fabsf(v.y) < epsilon;
    return tiny;
}

static inline Vector2 v2_negate(const Vector2 v)
{
    Vector2 result = { 0 };
    result.x = -v.x;
    result.y = -v.y;
    return result;
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

static inline Vector2 v2_scale(const Vector2 v, float factor)
{
    Vector2 result = v;
    result.x *= factor;
    result.y *= factor;
    return result;
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

static inline Vector2 v2_round(const Vector2 v)
{
    Vector2 result = { 0 };
    result.x = roundf(v.x);
    result.y = roundf(v.y);
    return result;
}

/// Vector3 ////////////////////////////////////////////////////////////////////

static inline Vector3 v3_init(float x, float y, float z)
{
    Vector3 result = { 0 };
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static inline int v3_is_zero(const Vector3 v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}

static inline int v3_is_tiny(const Vector3 v, float epsilon)
{
    int tiny = fabsf(v.x) < epsilon && fabsf(v.y) < epsilon && fabsf(v.z) < epsilon;
    return tiny;
}

static inline Vector3 v3_negate(const Vector3 v)
{
    Vector3 result = { 0 };
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

static inline Vector3 v3_add(const Vector3 a, const Vector3 b)
{
    Vector3 result = { 0 };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

static inline Vector3 v3_sub(const Vector3 a, const Vector3 b)
{
    Vector3 result = { 0 };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static inline float v3_length_sq(const Vector3 v)
{
    const float lengthSq = v.x * v.x + v.y * v.y + v.z * v.z;
    return lengthSq;
}

static inline float v3_length(const Vector3 v)
{
    const float length = sqrtf(v3_length_sq(v));
    return length;
}

static inline Vector3 v3_scale(const Vector3 v, float factor)
{
    Vector3 result = v;
    result.x *= factor;
    result.y *= factor;
    result.z *= factor;
    return result;
}

static inline Vector3 v3_normalize(const Vector3 v)
{
    if (v3_is_zero(v)) {
        return v;
    }

    const float length = v3_length(v);
    Vector3 result = { 0 };
    result.x = v.x / length;
    result.y = v.y / length;
    result.z = v.z / length;
    return result;
}

static inline float v3_dot(const Vector3 a, const Vector3 b)
{
    const float dot = a.x * b.x + a.y * b.y + a.z * b.z;
    return dot;
}

static inline float v3_distance_sq(const Vector3 a, const Vector3 b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    const float distance_sq = dx * dx + dy * dy + dz * dz;
    return distance_sq;
}

static inline float v3_distance(const Vector3 a, const Vector3 b)
{
    const float distance = sqrtf(v3_distance_sq(a, b));
    return distance;
}

static inline Vector3 v3_round(const Vector3 v)
{
    Vector3 result = { 0 };
    result.x = roundf(v.x);
    result.y = roundf(v.y);
    result.z = roundf(v.z);
    return result;
}
