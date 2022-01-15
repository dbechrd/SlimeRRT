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
    Grass2,
    Grass3,
    Count
};

// TODO: Refactor RRT logic out into standalone file
struct RRTVertex {
    TileType  tileType    {};
    Vector2i  position    {};
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
    int       width     {};  // width of map in tiles
    int       height    {};  // height of map in tiles
    RRT       rrt       {};  // "Rapidly-exploring Random Tree" data struture (used for procedural generation)
    Tile     *tiles     {};  // array of tile data
    Texture   minimap   {};
    TilesetID tilesetId {};
    Tilemap  *next      {};

    ~Tilemap();
    void GenerateMinimap (void);
    Tile *TileAt         (int tileX, int tileY) const;  // Return tile at grid position x,y, assert on failure
    Tile *TileAtTry      (int tileX, int tileY) const;  // Return tile at grid position x,y, if it exists
    Tile *TileAtWorld    (int pixelX, int pixelY, int *tileX, int *tileY) const;  // Return tile at pixel position in world space, assert on failure
    Tile *TileAtWorldTry (int pixelX, int pixelY, int *tileX, int *tileY) const;  // Return tile at pixel position in world space, if it exists
    Tile *Tilemap::TileAtWorldUnit(int unitX, int unitY, int *tileX, int *tileY) const;  // Return tile at sub-tile unit position
    Tile *Tilemap::TileAtWorldUnitTry(int unitX, int unitY, int *tileX, int *tileY) const;  // Return tile at sub-tile unit position
};

#define MAX_TILEMAPS 8

struct MapSystem {
    MapSystem(void);
    ~MapSystem(void);

    Tilemap *GenerateLobby (void);
    Tilemap *Generate      (dlb_rand32_t &rng, int width, int height);

private:
    Tilemap *Alloc           (void);
    void     BuildRRT        (Tilemap &map, dlb_rand32_t &rng, size_t numVertices, float maxGrowthDist);
    size_t   RRTNearestIndex (Tilemap &map, const Vector2i &p);

    Tilemap  maps[MAX_TILEMAPS] {};
    Tilemap *mapsFree           {};
    size_t   mapsActiveCount    {};
};