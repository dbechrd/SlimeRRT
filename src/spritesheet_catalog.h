#pragma once
#include "spritesheet.h"

enum class SpritesheetID {
    Empty,
    Charlie,
    Coin,
    Slime,
    Items,
    Count
};

namespace SpritesheetCatalog {
    static Spritesheet spritesheets[(int)SpritesheetID::Count];
    static void Load();
};