#pragma once
#include "Catalog.hpp"
#include "SFML/Audio.hpp"

class SoundBufferCatalog : public Catalog<sf::SoundBuffer>
{
public:
    enum Flags {
        // unused
    };

    void LoadDefaultBank();

private:
    bool onLoad(sf::SoundBuffer &soundBuffer, std::string path, int flags);
    sf::SoundBuffer &onMissing(std::string path);

    void LoadMissingOgg(sf::SoundBuffer &soundBuffer);
    static const unsigned char MISSING_OGG[7277];
};

namespace SndID {
    extern const std::string Banana;
    extern const std::string OmNomNom1;
    extern const std::string OmNomNom2;
    extern const std::string OmNomNom3;
}