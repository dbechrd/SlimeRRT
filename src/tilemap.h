#pragma once
#include "tileset.h"
#include "math.h"
#include "dlb_rand.h"

struct NetTile;

enum class TileType {
    Grass,
    Water,
    Forest,
    Wood,
    Concrete,
    Count
};

// TODO: Refactor RRT logic out into standalone file
struct RRTVertex {
    TileType  tileType    {};
    Vector2   position    {};
    Rectangle textureRect {};
};

struct RRT {
    size_t      vertexCount {};
    RRTVertex * vertices    {};
};

struct Tile {
    TileType tileType {};

    bool IsWalkable() const {
        return tileType != TileType::Water;
    }
};

struct Tilemap {
    size_t  width   {};  // width of map in tiles
    size_t  height  {};  // height of map in tiles
    RRT     rrt     {};  // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
    Tile *  tiles   {};  // array of tile data
    Texture minimap {};
};

void tilemap_generate_lobby (Tilemap &map);
void tilemap_generate_tiles (Tilemap &map, NetTile *&tiles, size_t tilesLength);
void tilemap_generate       (Tilemap &map, dlb_rand32_t &rng);
void tilemap_generate_ex    (Tilemap &map, dlb_rand32_t &rng, size_t width, size_t height);
void tilemap_free           (Tilemap &map);
Tile *tilemap_at            (Tilemap &map, int tileX, int tileY);  // Return tile at grid position x,y, assert on failure
Tile *tilemap_at_try        (Tilemap &map, int tileX, int tileY);  // Return tile at grid position x,y, if it exists
Tile *tilemap_at_world      (Tilemap &map, Tileset &tileset, float x, float y, int *tileX, int *tileY);  // Return tile at pixel position in world space, assert on failure
Tile *tilemap_at_world_try  (Tilemap &map, Tileset &tileset, float x, float y, int *tileX, int *tileY);  // Return tile at pixel position in world space, if it exists