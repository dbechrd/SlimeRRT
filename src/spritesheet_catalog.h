#pragma once
#include "spritesheet.h"

enum class SpritesheetID {
    Empty,
    Charlie,
    Coin,
    Slime,
    Count
};

struct SpritesheetCatalog
{
    Spritesheet spritesheets[(int)SpritesheetID::Count];

    void Load();
};

extern SpritesheetCatalog g_spritesheetCatalog;
