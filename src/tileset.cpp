#include "tileset.h"
#include "raylib/raylib.h"
#include <cassert>
#include <stdlib.h>

static Tileset tilesets[(size_t)TilesetID::Count];

static void tileset_load(TilesetID tilesetId, const char *texturePath);

void tileset_init(void)
{
    tileset_load(TilesetID::TS_Overworld, "resources/tiles32.png");
}

static void tileset_load(TilesetID tilesetId, const char *texturePath)
{
    Tileset &tileset = tilesets[(size_t)tilesetId];

    tileset.texture = LoadTexture(texturePath);
    assert(tileset.texture.width);
    assert((tileset.texture.width % TILE_W) == 0);

    int tilesPerRow = tileset.texture.width / TILE_W;
    for (size_t i = 0; i < (size_t)TileType::Count; i++) {
        tileset.textureRects[i].x = (float)(i % tilesPerRow * TILE_W);
        tileset.textureRects[i].y = (float)(i / tilesPerRow * TILE_W);
        tileset.textureRects[i].width = (float)TILE_W;
        tileset.textureRects[i].height = (float)TILE_W;
    }
}

Rectangle tileset_tile_rect(TilesetID tilesetId, TileType tileType)
{
    Tileset &tileset = tilesets[(size_t)tilesetId];

    Rectangle tileRect = tileset.textureRects[(size_t)tileType];
    return tileRect;
}

void tileset_draw_tile(TilesetID tilesetId, TileType tileType, Vector2 at)
{
    Tileset &tileset = tilesets[(size_t)tilesetId];

    Rectangle tileRect = tileset_tile_rect(tilesetId, tileType);
    DrawTextureRec(tileset.texture, tileRect, at, WHITE);
}