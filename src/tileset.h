#pragma once
#include "raylib.h"

struct Tileset {
    Texture *   texture      {};
    size_t      tileWidth    {};
    size_t      tileHeight   {};
    size_t      tileCount    {};
    size_t      tilesPerRow  {};
    Rectangle * textureRects {};

    Tileset(Texture *texture, size_t tileWidth, size_t tileHeight, size_t tileCount);
    ~Tileset();
};