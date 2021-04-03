#include "tileset.h"
#include "raylib.h"
#include <assert.h>
#include <stdlib.h>

void tileset_init(Tileset *tileset)
{
    assert(tileset);
    assert(tileset->texture);
    assert(tileset->texture->width);
    assert(tileset->tileWidth);
    assert(tileset->tileHeight);
    assert(tileset->tileCount);

    assert(tileset->texture->width % (int)tileset->tileWidth == 0);
    tileset->tilesPerRow = tileset->texture->width / tileset->tileWidth;

    tileset->textureRects = (Rectangle *)calloc(tileset->tileCount, sizeof(*tileset->textureRects));
    assert(tileset->textureRects);

    for (size_t i = 0; i < tileset->tileCount; i++) {
        tileset->textureRects[i].x = (float)(i % tileset->tilesPerRow * tileset->tileWidth);
        tileset->textureRects[i].y = (float)(i / tileset->tilesPerRow * tileset->tileHeight);
        tileset->textureRects[i].width  = (float)tileset->tileWidth;
        tileset->textureRects[i].height = (float)tileset->tileHeight;
    }
}

void tileset_init_ex(Tileset *tileset, Texture *texture, size_t tileWidth, size_t tileHeight, size_t tileCount)
{
    assert(tileset);
    assert(texture);
    assert(texture->width);
    assert(tileWidth);
    assert(tileHeight);
    assert(tileCount);

    tileset->texture = texture;
    tileset->tileWidth = tileWidth;
    tileset->tileHeight = tileHeight;
    tileset->tileCount = tileCount;
    tileset_init(tileset);
}

void tileset_free(Tileset *tileset)
{
    free(tileset->textureRects);
}