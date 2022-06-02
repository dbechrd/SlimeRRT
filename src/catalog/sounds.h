#pragma once
#include "raylib/raylib.h"

namespace Catalog {
    struct MasterMixer {
        float masterVolume = 1.0f;
        float musicVolume = 1.0f;
        float sfxVolume = 1.0f;
    } g_mixer;

    enum class SoundID {
        Empty,
        Footstep,
        Gold,
        Slime_Stab1,
        Squeak,
        Squish1,
        Squish2,
        Whoosh,
        GemBounce,
        Eughh,
        RainbowSparkles,
        Count
    };

    const char *SoundIDString(SoundID id);

    struct SoundMixer {
        float volumeLimit[(size_t)SoundID::Count]{};  // max volume (requested by user)
    };

    struct Sounds {
        void Load(void);
        void Unload(void);
        const Sound &FindById(SoundID id) const;
        void Play(SoundID id, float pitch = 1.0f, bool multi = false);
        bool Playing(SoundID id);
        const unsigned char *MissingOggData(size_t &fileSize);

        SoundMixer mixer{};
    private:
        Sound missingOggSound {};
        Sound byId            [(size_t)SoundID::Count]{};

        Sound MissingOggSound(void);
    } g_sounds;
}