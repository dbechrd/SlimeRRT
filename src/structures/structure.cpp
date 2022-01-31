#include "structure.h"
#include "../tilemap.h"
#include <cassert>

#include "vault.cpp"

void Structure::Spawn(Tilemap &map, float x, float y)
{
    for (uint32_t j = 0; j < g_structure_vault_h; j += TILE_W) {
        for (uint32_t i = 0; i < g_structure_vault_w; i += TILE_W) {
            Tile *tile = (Tile *)map.TileAtWorld(x+i, y+j);
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