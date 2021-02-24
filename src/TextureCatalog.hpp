#pragma once
#include "Catalog.hpp"
#include "SFML/Graphics.hpp"

namespace TexID {
    extern const std::string Grass;
    extern const std::string Items;
    extern const std::string Player;
    extern const std::string TextBackground;
    extern const std::string Tiles32;
    extern const std::string Tiles64;
}

class TextureCatalog : public Catalog<sf::Texture>
{
public:
    enum Flags {
        Repeat = 0b0001, ///< Enable texture repeating
    };

    void LoadDefaultBank();

private:
    bool onLoad(sf::Texture &texture, std::string path, int flags);
    sf::Texture &onMissing(std::string path);

    void LoadMissingPng(sf::Texture &texture);
    static const unsigned char MISSING_PNG[153];
};