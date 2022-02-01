#pragma once
#include "spritesheet.h"

namespace Catalog {
    enum class SpritesheetID {
        Empty,
        Charlie,
        Coin_Copper,
        Coin_Silver,
        Coin_Gilded,
        Slime,
        Items,
        Count
    };

    struct Spritesheets {
        void Load(void);
        const Spritesheet &FindById(SpritesheetID id) const;

    private:
        Spritesheet byId[(size_t)SpritesheetID::Count];
    } g_spritesheets;
}