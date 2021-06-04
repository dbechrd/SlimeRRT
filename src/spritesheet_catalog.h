#pragma once

enum SpritesheetID {
    SpritesheetID_Empty,
    SpritesheetID_Charlie,
    SpritesheetID_Coin,
    SpritesheetID_Slime,
    SpritesheetID_Count
};

void                       spritesheet_catalog_init();
const struct Spritesheet * spritesheet_catalog_find(enum SpritesheetID id);
void                       spritesheet_catalog_free();