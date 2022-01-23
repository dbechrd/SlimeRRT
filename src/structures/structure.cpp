#include "structure.h"
#include "../tilemap.h"
#include <cassert>

#include "vault.cpp"

void Structure::Spawn(Tilemap &map, uint32_t x, uint32_t y)
{
    assert(x < map.width);
    assert(y < map.height);

    for (uint32_t j = 0; j < g_structure_vault_h; j++) {
        for (uint32_t i = 0; i < g_structure_vault_w; i++) {
            Tile *tile = (Tile *)map.TileAtWorld((float)x+i, (float)y+j);
            if (tile) {
                switch (g_structure_vault[j * g_structure_vault_w + i]) {
                    case '.': continue;
                    case 'g': tile->type = Tile::Type::Grass; break;
                    case 'w': tile->type = Tile::Type::Water; break;
                    case 'x': tile->type = Tile::Type::Wood;  break;
                }
            }
        }
    }
}