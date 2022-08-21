#include "catalog/spritesheets.h"
#include "spritesheet.h"

namespace Catalog {
    void Spritesheets::Load(void)
    {
        // TODO: Load spritesheets from file
        byId[(size_t)SpritesheetID::Charlie    ].LoadFromFile("data/entity/character/charlie.txt");
        byId[(size_t)SpritesheetID::Slime      ].LoadFromFile("data/entity/monster/slime.txt");
        byId[(size_t)SpritesheetID::Coin_Copper].LoadFromFile("data/entity/item/coin_copper.txt");
        byId[(size_t)SpritesheetID::Coin_Silver].LoadFromFile("data/entity/item/coin_silver.txt");
        byId[(size_t)SpritesheetID::Coin_Gilded].LoadFromFile("data/entity/item/coin_gilded.txt");
        byId[(size_t)SpritesheetID::Items      ].LoadFromFile("data/entity/item/items.txt");
    }

    const Spritesheet &Spritesheets::FindById(SpritesheetID id) const
    {
        return byId[(size_t)id];
    }
}