#include "spritesheet_catalog.h"
#include "spritesheet.h"
#include "dlb_types.h"
#include <cassert>

SpritesheetCatalog g_spritesheetCatalog;

void SpritesheetCatalog::Load()
{
    // TODO: Load spritesheets from file
    spritesheets[SpritesheetID_Charlie].LoadFromFile("resources/charlie.txt");
    spritesheets[SpritesheetID_Coin   ].LoadFromFile("resources/coin_gold.txt");
    spritesheets[SpritesheetID_Slime  ].LoadFromFile("resources/slime.txt");
}