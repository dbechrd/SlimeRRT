#pragma once
#include "tileset.h"
#include "math.h"
#include "dlb_rand.h"

enum class TileType : uint8_t {
    Grass,
    Water,
    Forest,
    Wood,
    Concrete,
    Grass2,
    Grass3,
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

    inline bool IsWalkable() const {
        return true; //tileType != TileType::Water;
    }

    inline bool IsSwimmable() const {
        return tileType == TileType::Water;
    }
};

struct TileChunk {
    Tile tiles[16][16]{};
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

    Tilemap *GenerateLobby (void);
    Tilemap *Generate      (dlb_rand32_t &rng, uint32_t width, uint32_t height);

private:
    Tilemap *Alloc           (void);
    void     BuildRRT        (Tilemap &map, dlb_rand32_t &rng, Vector2 qinit, size_t numVertices, float maxGrowthDist);
    size_t   RRTNearestIndex (Tilemap &map, Vector2 p);

    Tilemap  maps[MAX_TILEMAPS] {};
    Tilemap *mapsFree           {};
    size_t   mapsActiveCount    {};
};