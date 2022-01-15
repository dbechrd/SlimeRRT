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
    Texture texture          {};
    Recti   textureRects[16] {};
    size_t  tileCount        {};
    size_t  tilesPerRow      {};
};

void tileset_init(void);
const Recti &tileset_tile_rect(TilesetID tilesetId, TileType tileType);
void tileset_draw_tile(TilesetID tilesetId, TileType tileType, const Vector2i &at);