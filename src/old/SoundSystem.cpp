#include "SoundSystem.hpp"
#include "SoundBufferCatalog.hpp"

void SoundSystem::PlaySound(const sf::SoundBuffer &buffer)
{
    // NOTE: Would be nice to replace the oldest, or lowest priority, sound with the newly requested sound if we reach
    // the limit. Would have to maintain e.g. start_time and priority and somehow.
    for (size_t i = 0; i < SOUND_SYSTEM_MAX_SOUNDS; i++) {
        if (sounds[i].getStatus() != sf::Sound::Playing) {
            sounds[i].setBuffer(buffer);
            sounds[i].play();
            break;
        }
    }
}