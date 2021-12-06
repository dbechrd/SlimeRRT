#include "catalog/musics.h"
#include "catalog/sounds.h"
#include "raylib/raylib.h"
#include <cassert>

namespace Catalog {
    void Musics::Load(void)
    {
        float mus_background_vmin = 0.02f;
        float mus_background_vmax = 0.2f;
        Music mus_background = LoadMusicStream("resources/fluquor_copyright.ogg");
        mus_background.looping = true;
        //PlayMusicStream(mus_background);
        SetMusicVolume(mus_background, mus_background_vmax);
        UpdateMusicStream(mus_background);

        float mus_whistle_vmin = 0.0f;
        float mus_whistle_vmax = 0.0f; //1.0f;
        Music mus_whistle = LoadMusicStream("resources/whistle.ogg");
        SetMusicVolume(mus_whistle, mus_whistle_vmin);
        mus_whistle.looping = true;

        float whistleAlpha = 0.0f;

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
            if (!byId[i].frameCount) {
                byId[i] = MissingOggMusic();
            }
        }
    }

    void Musics::Unload(void)
    {
        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            if (byId[i].frameCount) {
                UnloadMusicStream(byId[i]);
            }
        }
    }

    const Music &Musics::FindById(MusicID id) const
    {
        return byId[(size_t)id];
    }

    void Musics::Play(MusicID id, float pitch) const
    {
        assert((size_t)id >= 0);
        assert((size_t)id < ARRAY_SIZE(byId));
        if (!byId[(size_t)id].frameCount) {
            return;
        }

        if (IsMusicStreamPlaying(byId[(size_t)id])) {
            return;
        }

        SetMusicPitch(byId[(size_t)id], pitch);
        PlayMusicStream(byId[(size_t)id]);
    }

    bool Musics::Playing(MusicID id) const
    {
        assert((size_t)id >= 0);
        assert((size_t)id < ARRAY_SIZE(byId));
        if (!byId[(size_t)id].frameCount) {
            return false;
        }

        bool playing = IsMusicStreamPlaying(byId[(size_t)id]);
        return playing;
    }

    void Musics::Update(void) const
    {
        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            if (Playing((MusicID)i)) {
                UpdateMusicStream(byId[i]);
            }
        }
    }

    Music Musics::MissingOggMusic(void)
    {
        static Music missingOgg = {};
        if (!missingOgg.frameCount) {
            size_t missingOggSize = 0;
            const unsigned char *missingOggData = Catalog::g_sounds.MissingOggData(missingOggSize);
            missingOgg = LoadMusicStreamFromMemory(".ogg", (unsigned char *)missingOggData, (int)missingOggSize);
            assert(missingOgg.frameCount);
        }
        return missingOgg;
    }
}