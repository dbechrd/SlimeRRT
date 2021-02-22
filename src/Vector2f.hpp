#pragma once
#include "SFML/Graphics.hpp"

namespace Vector2f {
    extern const sf::Vector2f Zero;

    float        Dot        (const sf::Vector2f &left, const sf::Vector2f &right);
    bool         IsZero     (const sf::Vector2f &vector);
    float        Length     (const sf::Vector2f &vector);
    float        LengthSq   (const sf::Vector2f &vector);
    sf::Vector2f Normalize  (const sf::Vector2f &vector);
}