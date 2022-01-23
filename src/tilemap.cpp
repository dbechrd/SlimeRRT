#include "tilemap.h"
#include "dlb_rand.h"
#include "dlb_types.h"
#include "maths.h"
#include "net_message.h"
#include "raylib/raylib.h"
#include <cassert>
#include <float.h>
#include <stdlib.h>

Tilemap::~Tilemap()
{
    //free(tiles);
    free(rrt.vertices);
    UnloadTexture(minimap);
}

void Tilemap::GenerateMinimap(void)
{
    // Check for OpenGL context
    if (!IsWindowReady()) {
        return;
    }

    UnloadTexture(minimap);

    Image minimapImg{};
    // NOTE: This is the client-side world map. Fog of war until tile types known?
    minimapImg.width = 16;
    minimapImg.height = 16;
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
    tileColors[(int)TileType::Grass2] = GREEN;
    tileColors[(int)TileType::Grass3] = GREEN;

    Color *minimapPixel = (Color *)minimapImg.data;
    for (size_t y = 0; y < minimapImg.height; y++) {
        for (size_t x = 0; x < minimapImg.width; x++) {
            const Tile *tile = &tileChunks[0][0].tiles[y][x];
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

const Tile *Tilemap::TileAtWorld(float x, float y) const
{
    if (x < 0 ||
        y < 0 ||
        x >= (float)TILE_W * width ||
        y >= (float)TILE_W * height)
    {
        return 0;
    }

    const int tileXWorld = (int)x / TILE_W;
    const int tileYWorld = (int)y / TILE_W;
    const int chunkX = tileXWorld / 16;
    const int chunkY = tileYWorld / 16;
    const int tileXChunk = tileXWorld % 16;
    const int tileYChunk = tileYWorld % 16;
    assert(chunkX < 8);
    assert(chunkY < 8);
    assert(tileXChunk < 16);
    assert(tileYChunk < 16);
    return &tileChunks[chunkY][chunkX].tiles[tileYChunk][tileXChunk];
}

MapSystem::MapSystem(void)
{
    // Initialze intrusive free lists
    for (size_t i = 0; i < MAX_TILEMAPS; i++) {
        if (i < MAX_PARTICLES - 1) {
            maps[i].next = &maps[i + 1];
        }
    }
    mapsFree = maps;
}

MapSystem::~MapSystem(void)
{
    //memset(maps, 0, sizeof(maps));
    //mapsFree = 0;
}

Tilemap *MapSystem::GenerateLobby(void)
{
    Tilemap *map = Alloc();
    if (!map) {
        return 0;
    }

    map->width = TILE_W * 16 * 8;
    map->height = TILE_W * 16 * 8;
    map->tilesetId = TilesetID::TS_Overworld;

    for (int y = 0; y < (int)map->height; y++) {
        for (int x = 0; x < (int)map->width; x++) {
            const Vector2 position = v2_init((float)x, (float)y);
            Tile *tile = (Tile *)map->TileAtWorld((float)x * TILE_W, (float)y * TILE_W);
            assert(tile);
            if (tile) {
                const int cx = x - (int)map->width / 2;
                const int cy = y - (int)map->height / 2;
                const int island_radius = 16;
                if (cx*cx + cy*cy > island_radius*island_radius) {
                //const int border_width = 26;
                //if (y < border_width || x < border_width || y >= map->height - border_width || x >= map->width - border_width) {
                    tile->tileType = TileType::Water;
                } else {
                    tile->tileType = TileType::Concrete;
                }
            }
        }
    }

    map->GenerateMinimap();
    return map;
}

Tilemap *MapSystem::Generate(dlb_rand32_t &rng, uint32_t width, uint32_t height)
{
    assert(width);
    assert(height);
    Tilemap *map = Alloc();
    if (!map) {
        return 0;
    }

    map->width = width;
    map->height = height;
    map->tilesetId = TilesetID::TS_Overworld;

    // NOTE: These parameters are chosen somewhat arbitrarily at this point
    const Vector2 middle = v2_init(0.5f, 0.5f);
    const size_t rrtSamples = 512;
    const float maxGrowthDistance = map->width * 0.25f;
    BuildRRT(*map, rng, middle, rrtSamples, maxGrowthDistance);

    const Vector2 tileCenterOffset = v2_init(0.5f / map->width, 0.5f / map->height);
    for (int y = 0; y < (int)map->height; y++) {
        for (int x = 0; x < (int)map->width; x++) {
            const Vector2 position = v2_init((float)x / map->width, (float)y / map->height);
            const Vector2 center = v2_add(position, tileCenterOffset);
            const size_t nearestIdx = RRTNearestIndex(*map, center);

            Tile *tile = (Tile *)map->TileAtWorld((float)x * TILE_W, (float)y * TILE_W);
            assert(tile);
            if (tile) {
                assert(nearestIdx < map->rrt.vertexCount);
                tile->tileType = map->rrt.vertices[nearestIdx].tileType;
            }
        }
    }

    // Pass 2: Surround water with sand for @rusteel
    for (int y = 0; y < (int)map->height; y++) {
        for (int x = 0; x < (int)map->width; x++) {
            const Tile *tile = map->TileAtWorld((float)x * TILE_W, (float)y * TILE_W);
            assert(tile);
            if (tile && tile->tileType == TileType::Water) {
                static const int beachWidth = 2;
                for (int beachX = x-beachWidth; beachX <= x+beachWidth; beachX++) {
                    for (int beachY = y-beachWidth; beachY <= y+beachWidth; beachY++) {
                        if (beachX == 0 && beachY == 0) continue;  // this is the water tile
                        Tile *tile = (Tile *)map->TileAtWorld((float)beachX * TILE_W, (float)beachY * TILE_W);
                        if (tile && tile->tileType != TileType::Water && tile->tileType != TileType::Concrete) {
                            tile->tileType = TileType::Concrete;
                        }
                    }
                }
            }
        }
    }

    map->GenerateMinimap();
    return map;
}

Tilemap *MapSystem::Alloc(void)
{
    Tilemap *map = mapsFree;
    if (!map) {
        //assert(!"Tilemap pool is full");
        //TraceLog(LOG_ERROR, "Tilemap pool is full.\n");
        return 0;
    }

    mapsFree = map->next;
    mapsActiveCount++;

    return map;
}

void MapSystem::BuildRRT(Tilemap &map, dlb_rand32_t &rng, Vector2 qinit, size_t numVertices, float maxGrowthDist)
{
    float maxGrowthDistSq = maxGrowthDist * maxGrowthDist;

    map.rrt.vertexCount = numVertices;
    map.rrt.vertices = (RRTVertex *)calloc(map.rrt.vertexCount, sizeof(*map.rrt.vertices));
    assert(map.rrt.vertices);

    const int tileMin = 0;
    const int tileMax = (int)TileType::Count - 1;

    RRTVertex *vertex = map.rrt.vertices;
    vertex->tileType = (TileType)dlb_rand32i_range_r(&rng, tileMin, tileMax);
    vertex->position = qinit;
    vertex++;

    size_t randTiles = numVertices / 8;

    Vector2 qrand = {};
    for (size_t tileIdx = 1; tileIdx < map.rrt.vertexCount; tileIdx++) {
        qrand.x = (float)dlb_rand32f_range_r(&rng, 0.0f, 1.0f);
        qrand.y = (float)dlb_rand32f_range_r(&rng, 0.0f, 1.0f);

        size_t nearestIdx = RRTNearestIndex(map, qrand);
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

size_t MapSystem::RRTNearestIndex(Tilemap &map, Vector2 p)
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