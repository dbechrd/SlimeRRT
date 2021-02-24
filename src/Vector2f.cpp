#pragma once
#include "Vector2f.hpp"

namespace Vector2f {
    const sf::Vector2f Zero(0.0f, 0.0f);

    bool IsZero(const sf::Vector2f &vector)
    {
        return vector.x == 0.0f && vector.y == 0.0f;
    }

    float LengthSq(const sf::Vector2f &vector)
    {
        float lengthSq = vector.x * vector.x + vector.y * vector.y;
        return lengthSq;
    }

    float Length(const sf::Vector2f &vector)
    {
        float length = std::sqrtf(LengthSq(vector));
        return length;
    }

    sf::Vector2f Normalize(const sf::Vector2f &vector)
    {
        if (IsZero(vector)) {
            return vector;
        }

        float length = Length(vector);
        return sf::Vector2f(vector.x / length, vector.y / length);
    }

    float Dot(const sf::Vector2f &a, const sf::Vector2f &b)
    {
        float dot = a.x * b.x + a.y * b.y;
        return dot;
    }

    float DistanceSq(const sf::Vector2f &a, const sf::Vector2f &b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dist_sq = dx * dx + dy * dy;
        return dist_sq;
    }

    float Distance(const sf::Vector2f &a, const sf::Vector2f &b)
    {
        float dist = std::sqrtf(DistanceSq(a, b));
        return dist;
    }
};