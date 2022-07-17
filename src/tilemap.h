#pragma once
#include "tileset.h"
#include "math.h"
#include "dlb_rand.h"
#include "OpenSimplex2F.h"
#include <vector>
#include <unordered_map>

struct World;

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

typedef uint32_t ChunkHash;

struct Chunk {
    int16_t x{};  // chunk x offset
    int16_t y{};  // chunk y offset
    Tile tiles[CHUNK_W * CHUNK_W]{};  // 32x32 tiles per chunk

    inline ChunkHash Hash(void) const {
        return ((uint16_t)x << 16) | (uint16_t)y;
    };
    static inline ChunkHash Hash(int16_t x, int16_t y) {
        return ((uint16_t)x << 16) | (uint16_t)y;
    };
};

struct Tilemap {
    RRT                   rrt        {};  // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
    Texture               minimap    {};
    TilesetID             tilesetId  {};
    Tilemap              *next       {};
    OpenSimplexEnv       *ose        {};
    OpenSimplexGradients *osg        {};
    OpenSimplexGradients *osg2       {};
    std::vector<Chunk>    chunks     {};  // TODO: RingBuffer, this set will grow indefinitely
    std::unordered_map<ChunkHash, size_t> chunksIndex{};  // [x << 16 | y] -> idx into chunks array

    ~Tilemap                    ();
    void SeedSimplex            (int64_t seed);
    void FreeSimplex            (void);
    bool GenerateNoise          (Texture &tex);
    void GenerateMinimap        (Vector2 worldPos);
    const int16_t CalcChunk     (float world) const;
    const int16_t CalcChunkTile (float world) const;
    const Tile *TileAtWorld     (float x, float y) const;  // Return tile at pixel position in world space, assert on failure
    Chunk &FindOrGenChunk       (World &world, int16_t x, int16_t y);
};

#define MAX_TILEMAPS 8

struct MapSystem {
    MapSystem(void);
    ~MapSystem(void);

    Tilemap *Alloc();

private:
    Tilemap  maps[MAX_TILEMAPS] {};
    Tilemap *mapsFree           {};
    size_t   mapsActiveCount    {};
};