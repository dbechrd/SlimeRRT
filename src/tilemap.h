#pragma once
#include "tileset.h"
#include "math.h"

enum TileType {
    TileType_Grass,
    TileType_Water,
    TileType_Forest,
    TileType_Wood,
    TileType_Concrete,
    TileType_Count
};

// TODO: Refactor RRT logic out into standalone file
struct RRTVertex {
    TileType tileType;
    Vector2 position;
    Rectangle textureRect;
};

struct RRT {
    size_t vertexCount;
    RRTVertex *vertices;
};

struct Tile {
    TileType tileType;
    Vector2 position;
};

struct Tilemap {
    size_t widthTiles;   // width of map in tiles
    size_t heightTiles;  // height of map in tiles
    size_t tileCount;    // number of tiles in the map (widthTiles * heightTiles)
    Tile *tiles;         // array of tile data
    Tileset *tileset;    // tileset to use for rendering
    RRT rrt;             // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
};

bool tile_is_walkable       (const Tile *tile);   // Return true if player can walk on the tile (false if tile is null)

void tilemap_generate       (Tilemap *map);
void tilemap_generate_ex    (Tilemap *map, size_t width, size_t height, Tileset *tileset);
void tilemap_free           (Tilemap *map);
Tile *tilemap_at            (Tilemap *map, int tileX, int tileY);  // Return tile at grid position x,y, assert on failure
Tile *tilemap_at_try        (Tilemap *map, int tileX, int tileY);  // Return tile at grid position x,y, if it exists
Tile *tilemap_at_world      (Tilemap *map, int x, int y);          // Return tile at pixel position in world space, assert on failure
Tile *tilemap_at_world_try  (Tilemap *map, int x, int y);          // Return tile at pixel position in world space, if it exists
