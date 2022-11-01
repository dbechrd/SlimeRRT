#pragma once
#include "spritesheet.h"

namespace Catalog {
    enum class SpritesheetID {
        Empty,
        Character_Charlie,
        Environment_Forest,
        Item_Coins,
        Monster_Slime,
        Count
    };

    struct Spritesheets {
        void Load(void);
        const Spritesheet &FindById(SpritesheetID id) const;

    private:
        Spritesheet byId[(size_t)SpritesheetID::Count];
    };

    thread_local static Spritesheets g_spritesheets{};
}