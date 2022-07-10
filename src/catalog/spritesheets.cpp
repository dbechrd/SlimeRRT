#include "catalog/spritesheets.h"
#include "spritesheet.h"

namespace Catalog {
    void Spritesheets::Load(void)
    {
        // TODO: Load spritesheets from file
        byId[(size_t)SpritesheetID::Charlie    ].LoadFromFile("data/sht/chr/charlie.txt");
        byId[(size_t)SpritesheetID::Slime      ].LoadFromFile("data/sht/mon/slime.txt");
        byId[(size_t)SpritesheetID::Coin_Copper].LoadFromFile("data/sht/itm/coin_copper.txt");
        byId[(size_t)SpritesheetID::Coin_Silver].LoadFromFile("data/sht/itm/coin_silver.txt");
        byId[(size_t)SpritesheetID::Coin_Gilded].LoadFromFile("data/sht/itm/coin_gilded.txt");
        byId[(size_t)SpritesheetID::Items      ].LoadFromFile("data/sht/itm/items.txt");
    }

    const Spritesheet &Spritesheets::FindById(SpritesheetID id) const
    {
        return byId[(size_t)id];
    }
}