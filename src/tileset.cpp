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
    for (int i = 0; i < (int)TileType::Count; i++) {
        tileset.textureRects[i].x = i % tilesPerRow * TILE_W;
        tileset.textureRects[i].y = i / tilesPerRow * TILE_W;
        tileset.textureRects[i].width = TILE_W;
        tileset.textureRects[i].height = TILE_W;
    }
}

const Recti &tileset_tile_rect(TilesetID tilesetId, TileType tileType)
{
    Tileset &tileset = tilesets[(size_t)tilesetId];
    return tileset.textureRects[(size_t)tileType];
}

void tileset_draw_tile(TilesetID tilesetId, TileType tileType, const Vector2i &at)
{
    Tileset &tileset = tilesets[(int)tilesetId];
    const Recti &tileRect = tileset_tile_rect(tilesetId, tileType);
    const Rectangle tileRectf{ (float)tileRect.x, (float)tileRect.y, (float)tileRect.width, (float)tileRect.height };
    const Vector2 atf{ (float)at.x, (float)at.y };
    DrawTextureRec(tileset.texture, tileRectf, atf, WHITE);
}