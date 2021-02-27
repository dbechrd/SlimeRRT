#pragma once
#include "SFML/Graphics.hpp"

namespace Vector2f {
    extern const sf::Vector2f Zero;

    bool         IsZero     (const sf::Vector2f &vector);
    float        LengthSq   (const sf::Vector2f &vector);
    float        Length     (const sf::Vector2f &vector);
    sf::Vector2f Normalize  (const sf::Vector2f &vector);
    float        Dot        (const sf::Vector2f &a, const sf::Vector2f &b);
    float        DistanceSq (const sf::Vector2f &a, const sf::Vector2f &b);
    float        Distance   (const sf::Vector2f &a, const sf::Vector2f &b);
}