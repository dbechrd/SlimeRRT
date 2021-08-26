#include "tilemap.h"
#include "dlb_rand.h"
#include "maths.h"
#include "raylib.h"
#include "dlb_types.h"
#include <assert.h>
#include <float.h>
#include <stdlib.h>

static void rrt_build(Tilemap *map, dlb_rand32_t *rng, Vector2 qinit, size_t numVertices, float maxGrowthDist);
static size_t rrt_nearest_idx(Tilemap *map, Vector2 p);

void tilemap_generate(Tilemap *map, dlb_rand32_t *rng)
{
    assert(map);
    assert(map->width);
    assert(map->height);
    assert(map->tileWidth);
    assert(map->tileHeight);

    map->tiles = (Tile *)calloc(map->width * map->height, sizeof(*map->tiles));
    assert(map->tiles);

    // NOTE: These parameters are chosen somewhat arbitrarily at this point
    const Vector2 middle = v2_init(map->width * map->tileWidth / 2.0f, map->height * map->tileHeight / 2.0f);
    const size_t rrtSamples = 512;
    const float maxGrowthDistance = map->width * map->tileWidth / 4.0f;
    rrt_build(map, rng, middle, rrtSamples, maxGrowthDistance);

    const Vector2 tileCenterOffset = v2_init(map->tileWidth / 2.0f, map->tileHeight / 2.0f);
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            //tile.tileType = randomType(g_mersenne);
            //tile.tileType = (tileY * w + tileX) % tileTypeCount; // Debug: Use every tile sequentially
            const Vector2 position = v2_init((float)x * map->tileWidth, (float)y * map->tileHeight);
            const Vector2 center = v2_add(position, tileCenterOffset);
            const size_t nearestIdx = rrt_nearest_idx(map, center);

            Tile *tile = tilemap_at(map, x, y);
            tile->tileType = map->rrt.vertices[nearestIdx].tileType;
            tile->position = position;
        }
    }

    // Pass 2: Surround water with sand for @rusteel
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            TileType tileType = tilemap_at(map, x, y)->tileType;
            if (tileType == TileType::Water) {
                static const int beachWidth = 2;
                for (int beachX = x-beachWidth; beachX <= x+beachWidth; beachX++) {
                    for (int beachY = y-beachWidth; beachY <= y+beachWidth; beachY++) {
                        if (beachX == 0 && beachY == 0) continue;  // this is the water tile
                        Tile *tile = tilemap_at_try(map, beachX, beachY);
                        if (tile && tile->tileType != TileType::Water && tile->tileType != TileType::Concrete) {
                            tile->tileType = TileType::Concrete;
                        }
                    }
                }
            }
        }
    }
}

void tilemap_generate_ex(Tilemap *map, size_t width, size_t height, size_t tileWidth, size_t tileHeight, dlb_rand32_t *rng)
{
    assert(map);
    assert(width);
    assert(height);
    assert(tileWidth);
    assert(tileHeight);
    assert(rng);

    map->width = width;
    map->height = height;
    map->tileWidth = tileWidth;
    map->tileHeight = tileHeight;
    tilemap_generate(map, rng);
}

void tilemap_free(Tilemap *map)
{
    free(map->tiles);
}

Tile *tilemap_at(Tilemap *map, int tileX, int tileY)
{
    size_t idx = (size_t)tileY * map->width + tileX;
    assert(idx < (size_t)map->width * map->height);
    return &map->tiles[idx];
}

Tile *tilemap_at_try(Tilemap *map, int tileX, int tileY)
{
    Tile *tile = NULL;
    if (tileX >= 0 && tileY >= 0 && tileX < map->width && tileY < map->width) {
        size_t idx = (size_t)tileY * map->width + tileX;
        assert(idx < (size_t)map->width * map->height);
        tile = &map->tiles[idx];
    }
    return tile;
}

Tile *tilemap_at_world(Tilemap *map, int x, int y)
{
    assert(x >= 0);
    assert(y >= 0);
    assert(x < (int)map->tileWidth * map->width);
    assert(y < (int)map->tileHeight * map->height);

    int tileX = x / (int)map->tileWidth;
    int tileY = y / (int)map->tileHeight;
    return tilemap_at(map, tileX, tileY);
}

Tile *tilemap_at_world_try(Tilemap *map, int x, int y)
{
    if (x < 0 || y < 0 ||
        (size_t)x >= map->tileWidth * map->width ||
        (size_t)y >= map->tileHeight * map->height)
    {
        return 0;
    }

    int tileX = x / (int)map->tileWidth;
    int tileY = y / (int)map->tileHeight;
    return tilemap_at_try(map, tileX, tileY);
}

static void rrt_build(Tilemap *map, dlb_rand32_t *rng, Vector2 qinit, size_t numVertices, float maxGrowthDist)
{
    float maxGrowthDistSq = maxGrowthDist * maxGrowthDist;

    map->rrt.vertexCount = numVertices;
    map->rrt.vertices = (RRTVertex *)calloc(map->rrt.vertexCount, sizeof(*map->rrt.vertices));
    assert(map->rrt.vertices);

    const int tileMin = 0;
    const int tileMax = (int)TileType::Count - 2;

    RRTVertex *vertex = map->rrt.vertices;
    vertex->tileType = (TileType)dlb_rand32i_range_r(rng, tileMin, tileMax);
    vertex->position = qinit;
    vertex++;

    size_t randTiles = numVertices / 8;

    Vector2 qrand = {};
    for (size_t tileIdx = 1; tileIdx < map->rrt.vertexCount; tileIdx++) {
#if 1
        qrand.x = (float)dlb_rand32i_range_r(rng, 0, (int)(map->width * map->tileWidth));
        qrand.y = (float)dlb_rand32i_range_r(rng, 0, (int)(map->height * map->tileHeight));
#else
        // TODO: Try tile coords instead of pixels
        qrand.tileX = (float)GetRandomValue(0, map->width);
        qrand.tileY = (float)GetRandomValue(0, map->height);
#endif
        size_t nearestIdx = rrt_nearest_idx(map, qrand);
        Vector2 qnear = map->rrt.vertices[nearestIdx].position;
        Vector2 qnew = v2_sub(qrand, qnear);
#if 0
        bool constantIncrement = false;  // always increment by "maxGrowthDist"
        bool clampIncrement = true;     // increment at most by maxGrowthDist
        if (constantIncrement || (clampIncrement && Vector2::LengthSq(qnear) > maxGrowthDistSq)) {
            qnew = Vector2::Normalize(qrand - qnear) * maxGrowthDist;
        }
#else
        UNUSED(maxGrowthDistSq);
#endif
        qnew = v2_add(qnew, qnear);
        if (tileIdx < randTiles) {
            vertex->tileType = (TileType)dlb_rand32i_range_r(rng, tileMin, tileMax);
        } else {
            vertex->tileType = map->rrt.vertices[nearestIdx].tileType;
        }
        vertex->position = qnew;
        vertex++;
        //G.add_edge(qnear, qnew)
    }

    //for (size_t i = 0; i < rrt.size(); i++) {
    //    printf("rrt[%zu]: {%f, %f}\n", i, rrt[i].position.tileX, rrt[i].position.tileY);
    //}
}

static size_t rrt_nearest_idx(Tilemap *map, Vector2 p)
{
    assert(map->rrt.vertexCount);

    float minDistSq = FLT_MAX;
    size_t minIdx = 0;
    for (size_t i = 0; i < map->rrt.vertexCount; i++) {
        float distSq = v2_distance_sq(p, map->rrt.vertices[i].position);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            minIdx = i;
        }
    }
    return minIdx;
}