#pragma once
#include "raylib/raylib.h"

struct Tileset {
    Texture *   texture      {};
    Rectangle * textureRects {};
    size_t      tileWidth    {};
    size_t      tileHeight   {};
    size_t      tileCount    {};
    size_t      tilesPerRow  {};

    Tileset(Texture *texture, size_t tileWidth, size_t tileHeight, size_t tileCount);
    ~Tileset();
};