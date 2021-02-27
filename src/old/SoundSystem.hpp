#pragma once
#include "SFML/Audio.hpp"

#define SOUND_SYSTEM_MAX_SOUNDS 16

class SoundSystem {
public:
    void PlaySound(const sf::SoundBuffer &buffer);

private:
    sf::Sound sounds[SOUND_SYSTEM_MAX_SOUNDS];
};