#pragma once
#include "spritesheet.h"

enum SpritesheetID {
    SpritesheetID_Empty,
    SpritesheetID_Charlie,
    SpritesheetID_Coin,
    SpritesheetID_Slime,
    SpritesheetID_Count
};

struct SpritesheetCatalog
{
    Spritesheet spritesheets[SpritesheetID_Count];

    void Load();
};

extern SpritesheetCatalog g_spritesheetCatalog;
