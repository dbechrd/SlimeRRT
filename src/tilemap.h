#pragma once
#include "tileset.h"
#include "math.h"
#include "dlb_rand.h"

// TODO: Refactor RRT logic out into standalone file
struct RRTVertex {
    Tile::Type tileType    {};
    Vector2    position    {};
    Rectangle  textureRect {};
};

struct RRT {
    size_t      vertexCount {};
    RRTVertex * vertices    {};
};

struct TileChunk {
    Tile tiles[32][32]{};
};

struct Tilemap {
    uint32_t  width      {};  // width of map in tiles
    uint32_t  height     {};  // height of map in tiles
    RRT       rrt        {};  // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
    TileChunk tileChunks [8][8]{};
    Texture   minimap    {};
    TilesetID tilesetId  {};
    Tilemap  *next       {};

    ~Tilemap();
    void GenerateMinimap    (void);
    const Tile *TileAtWorld (float x, float y) const;  // Return tile at pixel position in world space, assert on failure
};

#define MAX_TILEMAPS 8

struct MapSystem {
    MapSystem(void);
    ~MapSystem(void);

    Tilemap *Alloc         (void);
    Tilemap *GenerateLobby (void);
    Tilemap *Generate      (dlb_rand32_t &rng, uint32_t width, uint32_t height);

private:
    void     BuildRRT        (Tilemap &map, dlb_rand32_t &rng, Vector2 qinit, size_t numVertices, float maxGrowthDist);
    size_t   RRTNearestIndex (Tilemap &map, Vector2 p, size_t randTiles);

    Tilemap  maps[MAX_TILEMAPS] {};
    Tilemap *mapsFree           {};
    size_t   mapsActiveCount    {};
};