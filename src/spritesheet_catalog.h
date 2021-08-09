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
public:
    void Load();

public:
    const Spritesheet &FindById(SpritesheetID id) const;

private:
    Spritesheet spritesheets[SpritesheetID_Count];
};

extern SpritesheetCatalog g_spritesheetCatalog;
