#pragma once
#include "raylib.h"

struct Tileset {
    Texture *texture;
    size_t tileWidth;
    size_t tileHeight;
    size_t tileCount;

    size_t tilesPerRow;
    Rectangle *textureRects;
};

void tileset_init    (Tileset *tileset);
void tileset_init_ex (Tileset *tileset, Texture *texture, size_t tileWidth, size_t tileHeight, size_t tileCount);
void tileset_free    (Tileset *tileset);