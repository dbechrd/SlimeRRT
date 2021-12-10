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

        mixer.masterVolume = 1.0f;
        mixer.bgMusicDuckTo = 0.1f;
        for (size_t i = 0; i < (size_t)MusicID::Count; i++) {
            mixer.volume      [i] = 1.0f;
            mixer.volumeMaster[i] = 1.0f;
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
                mixer.volumeSpeed[i] = CLAMP(mixer.volumeSpeed[i], 0.1f, 2.0f);
                if (mixer.volume[i] < mixer.volumeTarget[i]) {
                    mixer.volume[i] += dt * mixer.volumeSpeed[i];
                    mixer.volume[i] = CLAMP(mixer.volume[i], 0.0f, mixer.volumeTarget[i]);
                } else if (mixer.volume[i] > mixer.volumeTarget[i]) {
                    mixer.volume[i] -= dt * mixer.volumeSpeed[i];
                    mixer.volume[i] = CLAMP(mixer.volume[i], mixer.volumeTarget[i], 1.0f);
                }
                float newVolume = mixer.volume[i] * mixer.volumeMaster[i];

                // https://www.robotplanet.dk/audio/audio_gui_design/
                // float a = log(0.00001);        // where 0.00001 is min volume
                // float b = 1 / (log(1.0) - a);  // where 1.0 is max volume
                // float y = exp(a + x/b);
                // a == -5
                // b == 0.2
                // y = exp(x/0.2 - 5)
                // y = exp(5*x - 5)
                // 0.296 is the approximate intersection between y = 0.1x and exp(5x - 5), provides linear fall-off
                float logVol = newVolume < 0.296f ? 0.1f * newVolume : exp(5.0f * newVolume - 5.0f);
                //float logVol = newVolume;

                // https://www.dr-lex.be/info-stuff/volumecontrols.html
                // x^4 approximation with linear fall-off to zero below 0.1, not as smooth as exp
                //float logVol = MAX(0.1f * newVolume, newVolume * newVolume * newVolume * newVolume);
                logVol = CLAMP(logVol, 0.0f, 1.0f);
                printf("logVol: %f\n", logVol);

                SetMusicVolume(byId[i], logVol);
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