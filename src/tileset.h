#pragma once
#include "raylib/raylib.h"
#include "tile.h"

enum class TilesetID {
    TS_Overworld,
    Count
};

struct Tileset {
    Texture   texture      {};
    Rectangle textureRects [TileType_Count]{};
    size_t    tileCount    {};
    size_t    tilesPerRow  {};
};

extern Tileset g_tilesets[(size_t)TilesetID::Count];

void tileset_init(void);
const Rectangle &tileset_tile_rect(TilesetID tilesetId, TileType tileType);
void tileset_draw_tile(TilesetID tilesetId, TileType tileType, Vector2 at, Color tint);
