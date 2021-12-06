#include "catalog/musics.h"
#include "catalog/sounds.h"
#include "raylib/raylib.h"
#include <cassert>

namespace Catalog {
    void Musics::Load(void)
    {
        byId[(size_t)MusicID::Footstep   ] = LoadMusicStream("resources/footstep1.ogg");
        byId[(size_t)MusicID::Gold       ] = LoadMusicStream("resources/gold1.ogg");
        byId[(size_t)MusicID::Slime_Stab1] = LoadMusicStream("resources/slime_stab1.ogg");
        byId[(size_t)MusicID::Squeak     ] = LoadMusicStream("resources/squeak1.ogg");
        byId[(size_t)MusicID::Squish1    ] = LoadMusicStream("resources/squish1.ogg");
        byId[(size_t)MusicID::Squish2    ] = LoadMusicStream("resources/squish2.ogg");
        byId[(size_t)MusicID::Whoosh     ] = LoadMusicStream("resources/whoosh1.ogg");
        byId[(size_t)MusicID::GemBounce  ] = LoadMusicStream("resources/gem_bounce.wav");

        SetMusicVolume(byId[(size_t)MusicID::Whoosh], 0.3f);

        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            if (!byId[i].sampleCount) {
                byId[i] = MissingOggMusic();
            }
        }
    }

    void Musics::Unload(void)
    {
        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            if (byId[i].sampleCount) {
                UnloadMusicStream(byId[i]);
            }
        }
    }

    const Music &Musics::FindById(MusicID id) const
    {
        return byId[(size_t)id];
    }

    void Musics::Play(MusicID id, float pitch)
    {
        assert((size_t)id >= 0);
        assert((size_t)id < ARRAY_SIZE(byId));
        if (!byId[(size_t)id].sampleCount) {
            return;
        }

        if (IsMusicPlaying(byId[(size_t)id])) {
            return;
        }

        SetMusicPitch(byId[(size_t)id], pitch);
        PlayMusicStream(byId[(size_t)id]);
    }

    bool Musics::Playing(MusicID id)
    {
        assert((size_t)id >= 0);
        assert((size_t)id < ARRAY_SIZE(byId));
        if (!byId[(size_t)id].sampleCount) {
            return false;
        }

        bool playing = IsMusicPlaying(byId[(size_t)id]);
        return playing;
    }

    Music Musics::MissingOggMusic(void)
    {
        static Music missingOgg = {};
        if (!missingOgg.sampleCount) {
            size_t missingOggSize = 0;
            const unsigned char *missingOggData = Catalog::g_sounds.MissingOggData(missingOggSize);
            missingOgg = LoadMusicStreamFromMemory(".ogg", (unsigned char *)missingOggData, (int)missingOggSize);
            assert(missingOgg.sampleCount);
        }
        return missingOgg;
    }
}