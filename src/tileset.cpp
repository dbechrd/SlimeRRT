#include "tileset.h"
#include "raylib.h"
#include <assert.h>
#include <stdlib.h>

Tileset::Tileset(Texture *texture, size_t tileWidth, size_t tileHeight, size_t tileCount) :
    texture(texture), tileWidth(tileWidth), tileHeight(tileHeight), tileCount(tileCount)
{
    assert(texture);
    assert(texture->width);
    assert(tileWidth);
    assert(tileHeight);
    assert(tileCount);

    assert(texture->width % (int)tileWidth == 0);
    tilesPerRow = texture->width / tileWidth;

    textureRects = (Rectangle *)calloc(tileCount, sizeof(*textureRects));
    assert(textureRects);

    for (size_t i = 0; i < tileCount; i++) {
        textureRects[i].x = (float)(i % tilesPerRow * tileWidth);
        textureRects[i].y = (float)(i / tilesPerRow * tileHeight);
        textureRects[i].width = (float)tileWidth;
        textureRects[i].height = (float)tileHeight;
    }
}

Tileset::~Tileset()
{
    free(textureRects);
}