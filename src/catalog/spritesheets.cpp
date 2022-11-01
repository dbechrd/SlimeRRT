#include "catalog/spritesheets.h"
#include "spritesheet.h"

namespace Catalog {
    void Spritesheets::Load(void)
    {
        // TODO: Load spritesheets from file
        byId[(size_t)SpritesheetID::Character_Charlie ].LoadFromFile("data/entity/character/charlie.txt");
        byId[(size_t)SpritesheetID::Environment_Forest].LoadFromFile("data/entity/environment/forest.txt");
        byId[(size_t)SpritesheetID::Item_Coins        ].LoadFromFile("data/entity/item/coins.txt");
        byId[(size_t)SpritesheetID::Monster_Slime     ].LoadFromFile("data/entity/monster/slime.txt");
        //byId[(size_t)SpritesheetID::Items      ].LoadFromFile("data/entity/item/items.txt");
    }

    const Spritesheet &Spritesheets::FindById(SpritesheetID id) const
    {
        return byId[(size_t)id];
    }
}