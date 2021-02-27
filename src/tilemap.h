#pragma once
#include "tileset.h"
#include "vector2f.h"

typedef enum TileType {
    Tile_Grass,
    Tile_Water,
    Tile_Forest,
    Tile_Wood,
    Tile_Concrete,
    Tile_Count
} TileType;

// TODO: Refactor RRT logic out into standalone file
typedef struct RRTVertex {
    TileType tileType;
    Vector2 position;
    Rectangle textureRect;
} RRTVertex;

typedef struct RRT {
    size_t vertexCount;
    RRTVertex *vertices;
} RRT;

typedef struct Tile {
    TileType tileType;
    Vector2 position;
} Tile;

typedef struct Tilemap {
    size_t widthTiles;   // width of map in tiles
    size_t heightTiles;  // height of map in tiles
    size_t tileCount;    // number of tiles in the map (widthTiles * heightTiles)
    Tile *tiles;         // array of tile data
    Tileset *tileset;    // tileset to use for rendering
    RRT rrt;             // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
} Tilemap;

void tilemap_generate   (Tilemap *map);
void tilemap_generate_ex(Tilemap *map, size_t width, size_t height, Tileset *tileset);
void tilemap_free       (Tilemap *map);
Tile *tilemap_at        (Tilemap *map, int x, int y);
Tile *tilemap_at_try    (Tilemap *map, int x, int y);