#pragma once

namespace Catalog {
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
        Count
    };

    struct Sounds {
        void Load(void);
        void Unload(void);
        const Sound &FindById(SoundID id) const;
        void Play(SoundID id, float pitch);
        bool Playing(SoundID id);
        const unsigned char *Sounds::MissingOggData(size_t &fileSize);

    private:
        Sound byId[(size_t)SoundID::Count];

        Sound MissingOggSound(void);
        Wave Sounds::MissingOggWave(void);
    } g_sounds;
}