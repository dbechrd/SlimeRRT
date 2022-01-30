#pragma once
#include "tileset.h"
#include "math.h"
#include "dlb_rand.h"
#include <vector>
#include <unordered_map>

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

typedef uint32_t ChunkHash;

struct Chunk {
    int16_t x{};  // chunk x offset
    int16_t y{};  // chunk y offset
    Tile tiles[1024]{};  // 32x32 tiles per chunk

    inline ChunkHash Hash(void) {
        return ((uint16_t)x << 16) | (uint16_t)y;
    };
    static inline ChunkHash Hash(int16_t x, int16_t y) {
        return ((uint16_t)x << 16) | (uint16_t)y;
    };
};

struct Tilemap {
    uint32_t  width      {};  // width of map in tiles
    uint32_t  height     {};  // height of map in tiles
    RRT       rrt        {};  // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
    std::vector<Chunk> chunks{};  // loaded chunks
    std::unordered_map<ChunkHash, uint32_t> chunksIndex{};  // [x << 16 | y] -> idx into chunks array
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
    Tilemap *Generate      (dlb_rand32_t &rng, uint16_t chunkRadius);

private:
    void     BuildRRT        (Tilemap &map, dlb_rand32_t &rng, Vector2 qinit, size_t numVertices, float maxGrowthDist);
    size_t   RRTNearestIndex (Tilemap &map, Vector2 p, size_t randTiles);

    Tilemap  maps[MAX_TILEMAPS] {};
    Tilemap *mapsFree           {};
    size_t   mapsActiveCount    {};
};