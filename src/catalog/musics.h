#pragma once

namespace Catalog {
    enum class MusicID {
        Empty,
        CopyrightBG,
        Whistle,
        Count
    };

    struct MusicMixer {
        //--------------------------------
        // TODO: User-facing settings
        //--------------------------------
        float masterVolume  {};  // base multiplier for all other volumes
        float volumeMaster  [(size_t)MusicID::Count]{};  // max volume (requested by user)
        float volume        [(size_t)MusicID::Count]{};  // current volume
        float volumeTarget  [(size_t)MusicID::Count]{};  // @no-serialize target volume for currently active transition (e.g. fade)
        float volumeSpeed   [(size_t)MusicID::Count]{};  // @no-serialize TODO: Transition curves?
        //--------------------------------
        // Internal settings
        //--------------------------------
        // TODO: expose via config file
        float bgMusicDuckTo {};  // volume when ducked behind idle track (perhaps pre-mix these in the source file / transition track?)
    };

    struct Musics {
        void Load(void);
        void Unload(void);
        Music &FindById(MusicID id);
        void Play(MusicID id, float pitch) const;
        bool Playing(MusicID id) const;
        void Update(float dt);

        MusicMixer mixer{};
    private:
        Music byId[(size_t)MusicID::Count]{};
        Music Musics::MissingOggMusic(void);
    } g_musics;
}