#pragma once
#include "Vector2f.hpp"

namespace Vector2f {
    const sf::Vector2f Zero(0.0f, 0.0f);

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

    bool IsZero(const sf::Vector2f &vector)
    {
        return vector.x == 0.0f && vector.y == 0.0f;
    }

    sf::Vector2f Normalize(const sf::Vector2f &vector)
    {
        if (IsZero(vector)) {
            return vector;
        }

        float length = Length(vector);
        return sf::Vector2f(vector.x / length, vector.y / length);
    }

    float Dot(const sf::Vector2f &left, const sf::Vector2f &right)
    {
        float dot = left.x * right.x + left.y * right.y;
        return dot;
    }
};