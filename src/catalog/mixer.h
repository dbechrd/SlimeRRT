#pragma once

namespace Catalog
{
    struct MasterMixer {
        float masterVolume {};
        float musicVolume  {};
        float sfxVolume    {};

        void Load(void);
    } g_mixer;
}