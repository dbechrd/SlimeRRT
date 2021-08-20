#include "spritesheet_catalog.h"
#include "spritesheet.h"
#include "dlb_types.h"
#include <cassert>

SpritesheetCatalog g_spritesheetCatalog;

void SpritesheetCatalog::Load()
{
    // TODO: Load spritesheets from file
    spritesheets[(int)SpritesheetID::Charlie].LoadFromFile("resources/charlie.txt");
    spritesheets[(int)SpritesheetID::Coin   ].LoadFromFile("resources/coin_gold.txt");
    spritesheets[(int)SpritesheetID::Slime  ].LoadFromFile("resources/slime.txt");
}