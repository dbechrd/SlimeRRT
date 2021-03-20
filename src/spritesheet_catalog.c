#include "spritesheet_catalog.h"
#include "spritesheet.h"
#include "dlb_types.h"
#include <assert.h>

static Spritesheet spritesheetCatalog[SpritesheetID_Count];

void spritesheet_catalog_init()
{
    // TODO: Load spritesheets from file
    spritesheet_init(&spritesheetCatalog[SpritesheetID_Charlie], "resources/charlie.txt");
    spritesheet_init(&spritesheetCatalog[SpritesheetID_Coin   ], "resources/coin_gold.txt");
    spritesheet_init(&spritesheetCatalog[SpritesheetID_Slime  ], "resources/slime.txt");
}

const Spritesheet *spritesheet_catalog_find(SpritesheetID id)
{
    // TODO: Return null if invalid id?
    assert(id < ARRAY_SIZE(spritesheetCatalog));
    return &spritesheetCatalog[id];
}

void spritesheet_catalog_free()
{
    for (size_t i = 0; i < SpritesheetID_Count; i++) {
        spritesheet_free(&spritesheetCatalog[i]);
    }
}