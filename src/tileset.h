#pragma once
#include "raylib/raylib.h"
#include "tile.h"

enum class TilesetID {
    TS_Overworld,
    Count
};

struct Tileset {
    Texture     texture          {};
    Rectangle   textureRects     [(size_t)Tile::Type::Count]{};
    size_t      tileCount        {};
    size_t      tilesPerRow      {};
};

extern Tileset g_tilesets[(size_t)TilesetID::Count];

void tileset_init(void);
const Rectangle &tileset_tile_rect(TilesetID tilesetId, Tile::Type tileType);
void tileset_draw_tile(TilesetID tilesetId, Tile::Type tileType, Vector2 at);