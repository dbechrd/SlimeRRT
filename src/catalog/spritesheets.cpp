#include "catalog/spritesheets.h"
#include "spritesheet.h"

namespace Catalog {
    void Spritesheets::Load(void)
    {
        // TODO: Load spritesheets from file
        byId[(size_t)SpritesheetID::Charlie].LoadFromFile("resources/charlie.txt");
        byId[(size_t)SpritesheetID::Coin   ].LoadFromFile("resources/coin_gold.txt");
        byId[(size_t)SpritesheetID::Slime  ].LoadFromFile("resources/slime.txt");
        byId[(size_t)SpritesheetID::Items  ].LoadFromFile("resources/items.txt");
    }

    const Spritesheet &Spritesheets::FindById(SpritesheetID id) const
    {
        return byId[(size_t)id];
    }
}