#include "catalog/musics.h"
#include "catalog/sounds.h"
#include "raylib/raylib.h"
#include <cassert>

namespace Catalog {
    void Musics::Load(void)
    {
        byId[(size_t)MusicID::CopyrightBG] = LoadMusicStream("resources/fluquor_copyright.ogg");
        byId[(size_t)MusicID::Whistle    ] = LoadMusicStream("resources/whistle.ogg");

        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            if (!byId[i].frameCount) {
                byId[i] = MissingOggMusic();
            }
        }

        mixer.masterVolume = 0.2f;
        mixer.bgMusicDuckTo = 0.1f;
        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            mixer.volume      [i] = 1.0f;
            mixer.volumeLimit [i] = 1.0f;
            mixer.volumeSpeed [i] = 1.0f;
            mixer.volumeTarget[i] = 1.0f;
            byId[i].looping = true;  // TODO: Handle this case-by-case via some config?
        }

        PlayMusicStream(byId[(size_t)MusicID::CopyrightBG]);
    }

    void Musics::Unload(void)
    {
        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            if (byId[i].frameCount) {
                UnloadMusicStream(byId[i]);
            }
        }
    }

    Music &Musics::FindById(MusicID id)
    {
        return byId[(size_t)id];
    }

    void Musics::Play(MusicID id, float pitch) const
    {
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
        assert((size_t)id < ARRAY_SIZE(byId));
        if (!byId[(size_t)id].frameCount) {
            return false;
        }

        bool playing = IsMusicStreamPlaying(byId[(size_t)id]);
        return playing;
    }

    void Musics::Update(float dt)
    {
        SetMasterVolume(mixer.masterVolume);
        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            if (Playing((MusicID)i)) {
                mixer.volumeTarget[i] = CLAMP(mixer.volumeTarget[i], 0.0f, 1.0f);
                mixer.volumeSpeed[i] = CLAMP(mixer.volumeSpeed[i], 0.1f, 100.0f);
                if (mixer.volume[i] < mixer.volumeTarget[i]) {
                    mixer.volume[i] += dt * mixer.volumeSpeed[i];
                    mixer.volume[i] = CLAMP(mixer.volume[i], 0.0f, mixer.volumeTarget[i]);
                } else if (mixer.volume[i] > mixer.volumeTarget[i]) {
                    mixer.volume[i] -= dt * mixer.volumeSpeed[i];
                    mixer.volume[i] = CLAMP(mixer.volume[i], mixer.volumeTarget[i], 1.0f);
                }
                SetMusicVolume(byId[i], mixer.volume[i] * mixer.volumeLimit[i]);
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