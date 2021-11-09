#include "tilemap.h"
#include "dlb_rand.h"
#include "dlb_types.h"
#include "maths.h"
#include "net_message.h"
#include "raylib/raylib.h"
#include <cassert>
#include <float.h>
#include <stdlib.h>

static void rrt_build(Tilemap &map, dlb_rand32_t &rng, Vector2 qinit, size_t numVertices, float maxGrowthDist);
static size_t rrt_nearest_idx(Tilemap &map, Vector2 p);

void tilemap_generate_minimap(Texture &minimap, Tilemap &map)
{
    // Check for OpenGL context
    if (!IsWindowReady()) {
        return;
    }

    UnloadTexture(minimap);

    Image minimapImg{};
    // NOTE: This is the client-side world map. Fog of war until tile types known?
    minimapImg.width = (int)map.width;
    minimapImg.height = (int)map.height;
    minimapImg.mipmaps = 1;
    minimapImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    assert(sizeof(Color) == 4);
    minimapImg.data = calloc((size_t)minimapImg.width * minimapImg.height, sizeof(Color));
    assert(minimapImg.data);

    Color tileColors[(int)TileType::Count]{};
    tileColors[(int)TileType::Grass] = GREEN;
    tileColors[(int)TileType::Water] = SKYBLUE;
    tileColors[(int)TileType::Forest] = DARKGREEN;
    tileColors[(int)TileType::Wood] = BROWN;
    tileColors[(int)TileType::Concrete] = GRAY;

    const size_t height = map.height;
    const size_t width = map.width;

    Color *minimapPixel = (Color *)minimapImg.data;
    for (size_t y = 0; y < height; y += 1) {
        for (size_t x = 0; x < width; x += 1) {
            const Tile *tile = &map.tiles[y * map.width + x];
            // Draw all tiles as different colored pixels
            assert((int)tile->tileType >= 0);
            assert((int)tile->tileType < (int)TileType::Count);
            *minimapPixel = tileColors[(int)tile->tileType];
            minimapPixel++;
        }
    }

    minimap = LoadTextureFromImage(minimapImg);
    free(minimapImg.data);
}

void tilemap_generate_lobby(Tilemap &map)
{
    map.width = 84;
    map.height = 64;
    map.tilesetId = TilesetID::TS_Overworld;

    map.tiles = (Tile *)calloc(map.width * map.height, sizeof(*map.tiles));
    assert(map.tiles);

    for (int y = 0; y < (int)map.height; y++) {
        for (int x = 0; x < (int)map.width; x++) {
            const Vector2 position = v2_init((float)x, (float)y);
            Tile *tile = tilemap_at(map, x, y);
            const int cx = x - (int)map.width / 2;
            const int cy = y - (int)map.height / 2;
            const int island_radius = 16;
            if (cx*cx + cy*cy > island_radius*island_radius) {
            //const int border_width = 26;
            //if (y < border_width || x < border_width || y >= map.height - border_width || x >= map.width - border_width) {
                tile->tileType = TileType::Water;
            } else {
                tile->tileType = TileType::Concrete;
            }
        }
    }

    tilemap_generate_minimap(map.minimap, map);
}

void tilemap_generate_tiles(Tilemap &map, Tile *&tiles, size_t tilesLength)
{
    assert(map.width);
    assert(map.height);
    assert(map.width * map.height <= WORLD_MAX_TILES);
    assert(map.width * map.height == tilesLength);

    map.tiles = (Tile *)calloc(map.width * map.height, sizeof(*map.tiles));
    assert(map.tiles);

    for (int y = 0; y < (int)map.height; y++) {
        for (int x = 0; x < (int)map.width; x++) {
            const Vector2 position = v2_init((float)x, (float)y);
            Tile *tile = tilemap_at(map, x, y);
            tile->tileType = (TileType)tiles[x * map.width + y].tileType;
        }
    }

    tilemap_generate_minimap(map.minimap, map);
}

void tilemap_generate(Tilemap &map, dlb_rand32_t &rng)
{
    assert(map.width);
    assert(map.height);

    map.tiles = (Tile *)calloc(map.width * map.height, sizeof(*map.tiles));
    assert(map.tiles);

    // NOTE: These parameters are chosen somewhat arbitrarily at this point
    const Vector2 middle = v2_init(0.5f, 0.5f);
    const size_t rrtSamples = 512;
    const float maxGrowthDistance = map.width * 0.25f;
    rrt_build(map, rng, middle, rrtSamples, maxGrowthDistance);

    const Vector2 tileCenterOffset = v2_init(0.5f / map.width, 0.5f / map.height);
    for (int y = 0; y < (int)map.height; y++) {
        for (int x = 0; x < (int)map.width; x++) {
            const Vector2 position = v2_init((float)x / map.width, (float)y / map.height);
            const Vector2 center = v2_add(position, tileCenterOffset);
            const size_t nearestIdx = rrt_nearest_idx(map, center);

            Tile *tile = tilemap_at(map, x, y);
            tile->tileType = map.rrt.vertices[nearestIdx].tileType;
        }
    }

    // Pass 2: Surround water with sand for @rusteel
    for (int y = 0; y < (int)map.height; y++) {
        for (int x = 0; x < (int)map.width; x++) {
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

    tilemap_generate_minimap(map.minimap, map);
}

void tilemap_generate_ex(Tilemap &map, dlb_rand32_t &rng, uint32_t width, uint32_t height)
{
    assert(width);
    assert(height);
    assert(width * height <= WORLD_MAX_TILES);

    map.width = width;
    map.height = height;
    map.tilesetId = TilesetID::TS_Overworld;;
    tilemap_generate(map, rng);
}

void tilemap_free(Tilemap &map)
{
    free(map.tiles);
    UnloadTexture(map.minimap);
}

Tile *tilemap_at(Tilemap &map, int tileX, int tileY)
{
    size_t idx = (size_t)tileY * map.width + tileX;
    assert(idx < (size_t)map.width * map.height);
    return &map.tiles[idx];
}

Tile *tilemap_at_try(Tilemap &map, int tileX, int tileY)
{
    Tile *tile = NULL;
    if (tileX >= 0 && tileY >= 0 && tileX < (int)map.width && tileY < (int)map.width) {
        size_t idx = (size_t)tileY * map.width + tileX;
        assert(idx < (size_t)map.width * map.height);
        tile = &map.tiles[idx];
    }
    return tile;
}

Tile *tilemap_at_world(Tilemap &map, float x, float y, int *tileX, int *tileY)
{
    assert(x >= 0);
    assert(y >= 0);
    assert(x < (int)TILE_W * map.width);
    assert(y < (int)TILE_W * map.height);

    int tile_x = (int)x / (int)TILE_W;
    int tile_y = (int)y / (int)TILE_W;
    Tile *tile = tilemap_at(map, tile_x, tile_y);
    if (tile) {
        if (tileX) *tileX = tile_x;
        if (tileY) *tileY = tile_y;
    }
    return tile;
}

Tile *tilemap_at_world_try(Tilemap &map, float x, float y, int *tileX, int *tileY)
{
    if (x < 0 || y < 0 ||
        x >= (float)TILE_W * map.width ||
        y >= (float)TILE_W * map.height)
    {
        return 0;
    }

    int tile_x = (int)x / (int)TILE_W;
    int tile_y = (int)y / (int)TILE_W;
    Tile *tile = tilemap_at_try(map, tile_x, tile_y);
    if (tile) {
        if (tileX) *tileX = tile_x;
        if (tileY) *tileY = tile_y;
    }
    return tile;
}

static void rrt_build(Tilemap &map, dlb_rand32_t &rng, Vector2 qinit, size_t numVertices, float maxGrowthDist)
{
    float maxGrowthDistSq = maxGrowthDist * maxGrowthDist;

    map.rrt.vertexCount = numVertices;
    map.rrt.vertices = (RRTVertex *)calloc(map.rrt.vertexCount, sizeof(*map.rrt.vertices));
    assert(map.rrt.vertices);

    const int tileMin = 0;
    const int tileMax = (int)TileType::Count - 2;

    RRTVertex *vertex = map.rrt.vertices;
    vertex->tileType = (TileType)dlb_rand32i_range_r(&rng, tileMin, tileMax);
    vertex->position = qinit;
    vertex++;

    size_t randTiles = numVertices / 8;

    Vector2 qrand = {};
    for (size_t tileIdx = 1; tileIdx < map.rrt.vertexCount; tileIdx++) {
        qrand.x = (float)dlb_rand32f_range_r(&rng, 0.0f, 1.0f);
        qrand.y = (float)dlb_rand32f_range_r(&rng, 0.0f, 1.0f);

        size_t nearestIdx = rrt_nearest_idx(map, qrand);
        Vector2 qnear = map.rrt.vertices[nearestIdx].position;
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
            vertex->tileType = (TileType)dlb_rand32i_range_r(&rng, tileMin, tileMax);
        } else {
            vertex->tileType = map.rrt.vertices[nearestIdx].tileType;
        }
        vertex->position = qnew;
        vertex++;
    }
}

static size_t rrt_nearest_idx(Tilemap &map, Vector2 p)
{
    assert(map.rrt.vertexCount);

    float minDistSq = FLT_MAX;
    size_t minIdx = 0;
    for (size_t i = 0; i < map.rrt.vertexCount; i++) {
        float distSq = v2_distance_sq(p, map.rrt.vertices[i].position);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            minIdx = i;
        }
    }
    return minIdx;
}