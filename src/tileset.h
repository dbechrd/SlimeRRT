#pragma once
#include "raylib/raylib.h"

#define TILE_W 32
#define TILE_H TILE_W

enum class TileType;

enum class TilesetID {
    TS_Overworld,
    Count
};

struct Tileset {
    Texture     texture          {};
    Rectangle   textureRects[16] {};
    size_t      tileCount        {};
    size_t      tilesPerRow      {};
};

void tileset_init(void);
const Rectangle &tileset_tile_rect(TilesetID tilesetId, TileType tileType);
void tileset_draw_tile(TilesetID tilesetId, TileType tileType, Vector2 at);