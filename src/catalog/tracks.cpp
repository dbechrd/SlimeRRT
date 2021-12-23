#include "catalog/mixer.h"
#include "catalog/sounds.h"
#include "catalog/tracks.h"
#include "raylib/raylib.h"
#include <cassert>

namespace Catalog {
    const char *TrackIDString(TrackID id)
    {
        switch (id) {
            case TrackID::CopyrightBG: return "CopyrightBG";
            case TrackID::Whistle:     return "Whistle";
        }
        return 0;
    }

    void Tracks::Load(void)
    {
        byId[(size_t)TrackID::CopyrightBG] = LoadMusicStream("resources/fluquor_copyright.ogg");
        byId[(size_t)TrackID::Whistle    ] = LoadMusicStream("resources/whistle.ogg");

        for (size_t i = 0; i < (size_t)TrackID::Count; i++) {
            if (!byId[i].frameCount) {
                byId[i] = MissingOggTrack();
            }
            mixer.volumeLimit [i] = 1.0f;
            mixer.volume      [i] = 1.0f;
            mixer.volumeSpeed [i] = 1.0f;
            mixer.volumeTarget[i] = 1.0f;
            byId[i].looping = true;  // TODO: Handle this case-by-case via some config?
        }
        mixer.bgMusicDuckTo = 0.1f;

        PlayMusicStream(byId[(size_t)TrackID::CopyrightBG]);
    }

    void Tracks::Unload(void)
    {
        for (size_t i = 0; i < (size_t)TrackID::Count; i++) {
            if (byId[i].frameCount) {
                StopMusicStream(byId[i]);
                UnloadMusicStream(byId[i]);
            }
        }
    }

    Music &Tracks::FindById(TrackID id)
    {
        return byId[(size_t)id];
    }

    void Tracks::Play(TrackID id, float pitch) const
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

    bool Tracks::Playing(TrackID id) const
    {
        assert((size_t)id < ARRAY_SIZE(byId));
        if (!byId[(size_t)id].frameCount) {
            return false;
        }

        bool playing = IsMusicStreamPlaying(byId[(size_t)id]);
        return playing;
    }

    void Tracks::Update(float dt)
    {
        for (size_t i = 0; i < (size_t)TrackID::Count; i++) {
            if (Playing((TrackID)i)) {
                mixer.volumeTarget[i] = CLAMP(mixer.volumeTarget[i], 0.0f, 1.0f);
                mixer.volumeSpeed[i] = CLAMP(mixer.volumeSpeed[i], 0.1f, 2.0f);
                if (mixer.volume[i] < mixer.volumeTarget[i]) {
                    mixer.volume[i] += dt * mixer.volumeSpeed[i];
                    mixer.volume[i] = CLAMP(mixer.volume[i], 0.0f, mixer.volumeTarget[i]);
                } else if (mixer.volume[i] > mixer.volumeTarget[i]) {
                    mixer.volume[i] -= dt * mixer.volumeSpeed[i];
                    mixer.volume[i] = CLAMP(mixer.volume[i], mixer.volumeTarget[i], 1.0f);
                }
                float volume = mixer.volume[i] * mixer.volumeLimit[i] * g_mixer.musicVolume;
                SetMusicVolume(byId[i], VolumeCorrection(volume));
                UpdateMusicStream(byId[i]);
            }
        }
    }

    Music Tracks::MissingOggTrack(void)
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