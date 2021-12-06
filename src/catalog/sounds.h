#pragma once
#include "helpers.h"

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

    private:
        Sound LoadMissingOGG(void);
        Sound byId[(size_t)SoundID::Count];
    } g_sounds;
}