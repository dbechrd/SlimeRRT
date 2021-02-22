#pragma once
#include "Catalog.hpp"
#include "SFML/Graphics.hpp"

class TextureCatalog : public Catalog<sf::Texture>
{
public:
    enum Flags {
        Repeat = 0b0001, ///< Enable texture repeating
    };

    static const std::string textBackgroundTextureN;
    static const std::string grassTextureN         ;
    static const std::string playerTextureN        ;
    static const std::string itemsTextureN         ;

    void LoadDefaultBank();

private:
    bool onLoad(sf::Texture &texture, std::string path, int flags);

    static const unsigned char MISSING_PNG[153];
};