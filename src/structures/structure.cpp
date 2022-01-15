#include "structure.h"
#include "../tilemap.h"
#include <cassert>

#include "vault.cpp"

void Structure::Spawn(Tilemap &map, int x, int y)
{
    assert(x < map.width);
    assert(y < map.height);

    for (int j = 0; j < g_structure_vault_h; j++) {
        for (int i = 0; i < g_structure_vault_w; i++) {
            Tile *tile = map.TileAtTry(x+i, y+j);
            if (tile) {
                switch (g_structure_vault[j * g_structure_vault_w + i]) {
                    case '.': continue;
                    case 'g': tile->tileType = TileType::Grass; break;
                    case 'w': tile->tileType = TileType::Water; break;
                    case 'x': tile->tileType = TileType::Wood;  break;
                }
            }
        }
    }
}