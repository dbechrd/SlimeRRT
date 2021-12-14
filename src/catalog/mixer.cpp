#include "mixer.h"

namespace Catalog {
    void MasterMixer::Load(void)
    {
        masterVolume = 1.0f;
        musicVolume  = 1.0f;
        sfxVolume    = 1.0f;
    }
}