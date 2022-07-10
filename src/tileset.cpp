#include "tileset.h"
#include "raylib/raylib.h"
#include <cassert>
#include <stdlib.h>

Tileset g_tilesets[(size_t)TilesetID::Count];

static void tileset_load(TilesetID tilesetId, const char *texturePath);

void tileset_init(void)
{
    tileset_load(TilesetID::TS_Overworld, "data/img/til/tiles32.png");
}

static void tileset_load(TilesetID tilesetId, const char *texturePath)
{
    Tileset &tileset = g_tilesets[(size_t)tilesetId];

    tileset.texture = LoadTexture(texturePath);
    assert(tileset.texture.width);
    assert((tileset.texture.width % TILE_W) == 0);

    int tilesPerRow = tileset.texture.width / TILE_W;
    for (size_t i = 0; i < (size_t)TileType_Count; i++) {
        tileset.textureRects[i].x = (float)(i % tilesPerRow * TILE_W);
        tileset.textureRects[i].y = (float)(i / tilesPerRow * TILE_W);
        tileset.textureRects[i].width = (float)TILE_W;
        tileset.textureRects[i].height = (float)TILE_W;
    }
}

const Rectangle &tileset_tile_rect(TilesetID tilesetId, TileType tileType)
{
    Tileset &tileset = g_tilesets[(size_t)tilesetId];
    return tileset.textureRects[(size_t)tileType];
}

void tileset_draw_tile(TilesetID tilesetId, TileType tileType, Vector2 at, Color tint)
{
    Tileset &tileset = g_tilesets[(size_t)tilesetId];
    Rectangle tileRect = tileset_tile_rect(tilesetId, tileType);
    DrawTextureRec(tileset.texture, tileRect, at, tint);
}