#pragma once

namespace Catalog {
    enum class TrackID {
        Empty,
        CopyrightBG,
        Whistle,
        Count
    };

    const char *TrackIDString(TrackID id);

    struct TrackMixer {
        //--------------------------------
        // TODO: User-facing settings
        //--------------------------------
        float volumeLimit   [(size_t)TrackID::Count]{};  // max volume (requested by user)
        float volume        [(size_t)TrackID::Count]{};  // current volume
        float volumeTarget  [(size_t)TrackID::Count]{};  // @no-serialize target volume for currently active transition (e.g. fade)
        float volumeSpeed   [(size_t)TrackID::Count]{};  // @no-serialize TODO: Transition curves?
        //--------------------------------
        // Internal settings
        //--------------------------------
        // TODO: expose via config file
        float bgMusicDuckTo {};  // volume when ducked behind idle track (perhaps pre-mix these in the source file / transition track?)
    };

    struct Tracks {
        void Load(void);
        void Unload(void);
        Music &FindById(TrackID id);
        void Play(TrackID id, float pitch) const;
        bool Playing(TrackID id) const;
        void Update(float dt);

        TrackMixer mixer{};
    private:
        Music byId[(size_t)TrackID::Count]{};
        Music MissingOggTrack(void);
    } g_tracks;
}