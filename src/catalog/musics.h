#pragma once

namespace Catalog {
    enum class MusicID {
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

    struct Musics {
        void Load(void);
        void Unload(void);
        const Music &FindById(MusicID id) const;
        void Play(MusicID id, float pitch) const;
        bool Playing(MusicID id) const;
        void Update(void) const;

    private:
        Music Musics::MissingOggMusic(void);
        Music byId[(size_t)MusicID::Count];
    } g_musics;
}