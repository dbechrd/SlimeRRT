#pragma once
#include <cmath>
#include "dlb_types.h"

typedef struct Vector2i {
    int x;
    int y;
} Vector2i;

typedef struct Vector3i {
    int x;
    int y;
    int z;
} Vector3i;

typedef struct Recti {
    int x;
    int y;
    int width;
    int height;
} Recti;

/// Rect ///////////////////////////////////////////////////////////////////////

static inline Recti RectPad(const Recti &rec, int pad)
{
    Recti padded{ rec.x - pad, rec.y - pad, rec.width + pad * 2, rec.height + pad * 2 };
    return padded;
}

static inline Recti RectPadXY(const Recti &rec, int padX, int padY)
{
    Recti padded{ rec.x - padX, rec.y - padY, rec.width + padX * 2, rec.height + padY * 2 };
    return padded;
}

static inline bool CheckCollisionRecti(const Recti &rec1, const Recti &rec2)
{
    const bool collision =
        (rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
        (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y);
    return collision;
}

static inline bool PointInRect(const Vector2i &point, const Recti &rect)
{
    return (point.x >= rect.x && point.y >= rect.y && point.x < rect.x + rect.width && point.y < rect.y + rect.height);
}

bool CheckCollisionCircleRecti(const Vector2i &center, int radius, const Recti &rec);

#if 0
/// Vector2 ////////////////////////////////////////////////////////////////////

static inline Vector2 v2_init(float x, float y)
{
    Vector2 result{ x, y };
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
    Vector2 result{ -v.x, -v.y };
    return result;
}

static inline int v2_equal(const Vector2 a, const Vector2 b)
{
    int equal = a.x == b.x && a.y == b.y;
    return equal;
}

static inline Vector2 v2_add(const Vector2 a, const Vector2 b)
{
    Vector2 result{ a.x + b.x, a.y + b.y };
    return result;
}

static inline Vector2 v2_sub(const Vector2 a, const Vector2 b)
{
    Vector2 result{ a.x - b.x, a.y - b.y };
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
    Vector2 result{ v.x * factor, v.y * factor };
    return result;
}

static inline Vector2 v2_normalize(const Vector2 v)
{
    if (v2_is_zero(v)) {
        return v;
    }

    const float invLength = 1.0f / v2_length(v);
    Vector2 result{ v.x * invLength, v.y * invLength };
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
    Vector2 result{ roundf(v.x), roundf(v.y) };
    return result;
}

/// Vector3 ////////////////////////////////////////////////////////////////////

static inline Vector3 v3_init(float x, float y, float z)
{
    Vector3 result{ x, y, z };
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
    Vector3 result{ -v.x, -v.y, -v.z };
    return result;
}

static inline int v3_equal(const Vector3 a, const Vector3 b)
{
    int equal = a.x == b.x && a.y == b.y && a.z == b.z;
    return equal;
}

static inline Vector3 v3_add(const Vector3 a, const Vector3 b)
{
    Vector3 result{ a.x + b.x, a.y + b.y, a.z + b.z };
    return result;
}

static inline Vector3 v3_sub(const Vector3 a, const Vector3 b)
{
    Vector3 result{ a.x - b.x, a.y - b.y, a.z - b.z };
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
    Vector3 result = { v.x * factor, v.y * factor, v.z * factor };
    return result;
}

static inline Vector3 v3_normalize(const Vector3 v)
{
    if (v3_is_zero(v)) {
        return v;
    }

    const float invLength = 1.0f / v3_length(v);
    Vector3 result{ v.x * invLength, v.y * invLength, v.z * invLength };
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
    Vector3 result{ roundf(v.x), roundf(v.y), roundf(v.z) };
    return result;
}
#endif

/// Vector2i ////////////////////////////////////////////////////////////////////

static inline Vector2i v2_init(int x, int y)
{
    Vector2i result{ x, y };
    return result;
}

static inline int v2_is_zero(const Vector2i v)
{
    return v.x == 0 && v.y == 0;
}

static inline Vector2i v2_negate(const Vector2i v)
{
    Vector2i result{ -v.x, -v.y };
    return result;
}

static inline int v2_equal(const Vector2i a, const Vector2i b)
{
    int equal = a.x == b.x && a.y == b.y;
    return equal;
}

static inline Vector2i v2_add(const Vector2i a, const Vector2i b)
{
    Vector2i result{ a.x + b.x, a.y + b.y };
    return result;
}

static inline Vector2i v2_sub(const Vector2i a, const Vector2i b)
{
    Vector2i result{ a.x - b.x, a.y - b.y };
    return result;
}

static inline int v2_length_sq(const Vector2i v)
{
    const int lengthSq = v.x * v.x + v.y * v.y;
    return lengthSq;
}

static inline int v2_length(const Vector2i v)
{
    const int length = (int)sqrtf((float)v2_length_sq(v));
    return length;
}

static inline Vector2i v2_scale(const Vector2i v, int factor)
{
    Vector2i result{ v.x * factor, v.y * factor };
    return result;
}

static inline Vector2i v2_normalize(const Vector2i v)
{
    if (v2_is_zero(v)) {
        return v;
    }

    const float invLength = 1.0f / v2_length(v);
    Vector2i result{ (int)(v.x * invLength), (int)(v.y * invLength) };
    return result;
}

static inline int v2_dot(const Vector2i a, const Vector2i b)
{
    const int dot = a.x * b.x + a.y * b.y;
    return dot;
}

static inline int v2_distance_sq(const Vector2i a, const Vector2i b)
{
    const int dx = a.x - b.x;
    const int dy = a.y - b.y;
    const int distance_sq = dx * dx + dy * dy;
    return distance_sq;
}

static inline int v2_distance(const Vector2i a, const Vector2i b)
{
    const int distance = (int)sqrtf((float)v2_distance_sq(a, b));
    return distance;
}

/// Vector3i ////////////////////////////////////////////////////////////////////

static inline Vector3i v3_init(int x, int y, int z)
{
    Vector3i result{ x, y, z };
    return result;
}

static inline int v3_is_zero(const Vector3i v)
{
    return v.x == 0 && v.y == 0 && v.z == 0;
}

static inline Vector3i v3_negate(const Vector3i v)
{
    Vector3i result{ -v.x, -v.y, -v.z };
    return result;
}

static inline int v3_equal(const Vector3i a, const Vector3i b)
{
    int equal = a.x == b.x && a.y == b.y && a.z == b.z;
    return equal;
}

static inline Vector3i v3_add(const Vector3i a, const Vector3i b)
{
    Vector3i result{ a.x + b.x, a.y + b.y, a.z + b.z };
    return result;
}

static inline Vector3i v3_sub(const Vector3i a, const Vector3i b)
{
    Vector3i result{ a.x - b.x, a.y - b.y, a.z - b.z };
    return result;
}

static inline int v3_length_sq(const Vector3i v)
{
    const int lengthSq = v.x * v.x + v.y * v.y + v.z * v.z;
    return lengthSq;
}

static inline int v3_length(const Vector3i v)
{
    const int length = (int)sqrtf((float)v3_length_sq(v));
    return length;
}

static inline Vector3i v3_scale(const Vector3i v, int factor)
{
    Vector3i result = { v.x * factor, v.y * factor, v.z * factor };
    return result;
}

static inline Vector3i v3_scale_inv(const Vector3i v, int factor)
{
    if (factor == 0) {
        return { 0, 0, 0 };
    }
    Vector3i result = { v.x / factor, v.y / factor, v.z / factor };
    return result;
}

static inline Vector3i v3_normalize(const Vector3i v)
{
    if (v3_is_zero(v)) {
        return v;
    }

    const float invLength = 1.0f / v3_length(v);
    Vector3i result{ (int)(v.x * invLength), (int)(v.y * invLength), (int)(v.z * invLength) };
    return result;
}

static inline int v3_dot(const Vector3i a, const Vector3i b)
{
    const int dot = a.x * b.x + a.y * b.y + a.z * b.z;
    return dot;
}

static inline int v3_distance_sq(const Vector3i a, const Vector3i b)
{
    const int dx = a.x - b.x;
    const int dy = a.y - b.y;
    const int dz = a.z - b.z;
    const int distance_sq = dx * dx + dy * dy + dz * dz;
    return distance_sq;
}

static inline int v3_distance(const Vector3i a, const Vector3i b)
{
    const int distance = (int)sqrtf((float)v3_distance_sq(a, b));
    return distance;
}

/// AABB ///////////////////////////////////////////////////////////////////////

struct AABB {
    Vector2i min{};
    Vector2i max{};

    static int wastedSpace(const AABB &a, const AABB &b)
    {
        AABB u = a.calcUnion(b);
        int wastedSpace = u.calcArea() - (a.calcArea() + b.calcArea());
        return wastedSpace;
    }

    bool intersects(const AABB &other) const
    {
        return max.x > other.min.x &&
            min.x < other.max.x &&
            max.y > other.min.y &&
            min.y < other.max.y;
    }

    int calcArea() const
    {
        int area = (max.x - min.x) * (max.y - min.y);
        return area;
    }

    AABB calcUnion(const AABB &other) const
    {
        AABB u{
            { MIN(min.x, other.min.x), MIN(min.y, other.min.y) },
            { MAX(max.x, other.max.x), MAX(max.y, other.max.y) }
        };
        return u;
    }

    int calcAreaIncrease(const AABB &other) const
    {
        int area = calcArea();
        AABB newAABB = calcUnion(other);
        int newArea = newAABB.calcArea();
        int areaIncrease = (newArea > area) * (newArea - area);
        return areaIncrease;
    }

    void growToContain(const AABB &other)
    {
        min.x = MIN(min.x, other.min.x);
        min.y = MIN(min.y, other.min.y);
        max.x = MAX(max.x, other.max.x);
        max.y = MAX(max.y, other.max.y);
    }

    Recti toRect() const
    {
        Recti rect{
            min.x,
            min.y,
            max.x - min.x,
            max.y - min.y
        };
        return rect;
    }
};