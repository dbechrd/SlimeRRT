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
    uint32_t  width     {};  // width of map in tiles
    uint32_t  height    {};  // height of map in tiles
    RRT       rrt       {};  // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
    Tile     *tiles     {};  // array of tile data
    Texture   minimap   {};
    TilesetID tilesetId {};
    Tilemap  *next      {};

    ~Tilemap();
    void GenerateMinimap (void);
    void SyncTiles       (const Tile *tiles, size_t tilesLength);

    Tile *TileAt         (int tileX, int tileY);  // Return tile at grid position x,y, assert on failure
    Tile *TileAtTry      (int tileX, int tileY);  // Return tile at grid position x,y, if it exists
    Tile *TileAtWorld    (float x, float y, int *tileX, int *tileY);  // Return tile at pixel position in world space, assert on failure
    Tile *TileAtWorldTry (float x, float y, int *tileX, int *tileY);  // Return tile at pixel position in world space, if it exists
};

#define MAX_TILEMAPS 8

struct MapSystem {
    MapSystem(void);
    ~MapSystem(void);

    size_t MapsActive(void);

    Tilemap *GenerateLobby     (void);
    Tilemap *GenerateFromTiles (Tile *&tiles, size_t tilesLength);
    Tilemap *Generate          (dlb_rand32_t &rng, uint32_t width, uint32_t height);

private:
    Tilemap *Alloc           (void);
    void     BuildRRT        (Tilemap &map, dlb_rand32_t &rng, Vector2 qinit, size_t numVertices, float maxGrowthDist);
    size_t   RRTNearestIndex (Tilemap &map, Vector2 p);

    Tilemap  maps[MAX_TILEMAPS] {};
    Tilemap *mapsFree           {};
    size_t   mapsActiveCount    {};
};