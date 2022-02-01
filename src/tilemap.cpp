#include "tilemap.h"
#include "dlb_rand.h"
#include "dlb_types.h"
#include "maths.h"
#include "net_message.h"
#include "OpenSimplex2F.h"
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

bool Tilemap::GenerateNoise(Texture &tex)
{
    // Check for OpenGL context
    if (!IsWindowReady()) {
        return false;
    }

    Color tileColors[(int)Tile::Type::Count]{};
    tileColors[(int)Tile::Type::Void] = BLACK;
    tileColors[(int)Tile::Type::Grass] = GREEN;
    tileColors[(int)Tile::Type::Water] = SKYBLUE;
    tileColors[(int)Tile::Type::Forest] = DARKGREEN;
    tileColors[(int)Tile::Type::Wood] = BROWN;
    tileColors[(int)Tile::Type::Concrete] = GRAY;
    tileColors[(int)Tile::Type::Grass2] = GREEN;
    tileColors[(int)Tile::Type::Grass3] = GREEN;

    thread_local OpenSimplexEnv *ose = 0;
    if (!ose) ose = initOpenSimplex();
    thread_local OpenSimplexGradients *osg = 0;
    if (!osg) osg = newOpenSimplexGradients(ose, 1234);

#define WIDTH 256 //4096
#define HEIGHT 256 //4096
#define PERIOD 64.0
#define OFF_X 2048
#define OFF_Y 2048
#define FREQ 1.0 / PERIOD

    Image minimapImg{};
    minimapImg.width = WIDTH;
    minimapImg.height = HEIGHT;
    minimapImg.mipmaps = 1;
    minimapImg.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE; //PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    minimapImg.data = calloc(WIDTH * HEIGHT, sizeof(uint8_t));
    assert(minimapImg.data);

    uint8_t *minimapPixel = (uint8_t *)minimapImg.data;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            const double noise = noise2(ose, osg, (x + OFF_X) * FREQ, (y + OFF_Y) * FREQ);
            //printf("%0.2f ", noise);

            *minimapPixel = (uint8_t)(128.0 + noise * 128.0); // tileColors[(int)tile.type];
            //printf("%d ", *minimapPixel);
            minimapPixel++;
        }
        //printf("\n");
    }

#undef WIDTH
#undef HEIGHT
#undef PERIOD
#undef OFF_X
#undef OFF_Y
#undef FREQ

    tex = LoadTextureFromImage(minimapImg);
    free(minimapImg.data);
    return true;
}

void Tilemap::GenerateMinimap(void)
{
    return;

    // Check for OpenGL context
    if (!IsWindowReady()) {
        return;
    }

    UnloadTexture(minimap);

    Image minimapImg{};
    // NOTE: This is the client-side world map. Fog of war until tile types known?
    minimapImg.width = CHUNK_W * 8;
    minimapImg.height = CHUNK_W * 8;
    minimapImg.mipmaps = 1;
    minimapImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    assert(sizeof(Color) == 4);
    minimapImg.data = calloc((size_t)minimapImg.width * minimapImg.height, sizeof(Color));
    assert(minimapImg.data);

    Color tileColors[(int)Tile::Type::Count]{};
    tileColors[(int)Tile::Type::Void] = BLACK;
    tileColors[(int)Tile::Type::Grass] = GREEN;
    tileColors[(int)Tile::Type::Water] = SKYBLUE;
    tileColors[(int)Tile::Type::Forest] = DARKGREEN;
    tileColors[(int)Tile::Type::Wood] = BROWN;
    tileColors[(int)Tile::Type::Concrete] = GRAY;
    tileColors[(int)Tile::Type::Grass2] = GREEN;
    tileColors[(int)Tile::Type::Grass3] = GREEN;

    // TODO: Calculate chunk x,y for chunk -4,-4 from player's position, to chunk +4, +4
    // Also, make sure to put pixels in minimapPixel in the correct location
    Color *minimapPixel = (Color *)minimapImg.data;
    for (const Chunk &chunk : chunks) {
        for (size_t i = 0; i < ARRAY_SIZE(chunk.tiles); i++) {
            const Tile &tile = chunk.tiles[i];
            // Draw all tiles as different colored pixels
            assert((int)tile.type >= 0);
            assert((int)tile.type < (int)Tile::Type::Count);
            *minimapPixel = tileColors[(int)tile.type];
            minimapPixel++;
        }
    }

    minimap = LoadTextureFromImage(minimapImg);
    free(minimapImg.data);
}

const int16_t Tilemap::CalcChunk(float world) const
{
    float chunk = floorf(world / CHUNK_W / TILE_W);
    return (int16_t)chunk;
}

const int16_t Tilemap::CalcChunkTile(float world) const
{
    const float chunk = CalcChunk(world);
    const float chunkStart = chunk * CHUNK_W * TILE_W;
    const float chunkOffset = world - chunkStart;
    const float tilef = CLAMP(floorf(chunkOffset / TILE_W), 0, CHUNK_W - 1);
    //const float tilef = floorf(chunkOffset / TILE_W);
    int16_t tile = (int16_t)tilef;
    DLB_ASSERT(tile >= 0);
    DLB_ASSERT(tile < CHUNK_W);
    return tile;
}

const Tile *Tilemap::TileAtWorld(float x, float y) const
{
    assert(CalcChunk(0) == 0);
    assert(CalcChunk(1) == 0);
    assert(CalcChunk(CHUNK_W * TILE_W - 1) == 0);
    assert(CalcChunk(CHUNK_W * TILE_W) == 1);

    assert(CalcChunk(-1) == -1);
    assert(CalcChunk(-(CHUNK_W * TILE_W - 1)) == -1);
    assert(CalcChunk(-(CHUNK_W * TILE_W)) == -1);
    assert(CalcChunk(-(CHUNK_W * TILE_W + 1)) == -2);

    assert(CalcChunkTile(0) == 0);
    assert(CalcChunkTile(1) == 0);
    assert(CalcChunkTile(TILE_W - 1) == 0);
    assert(CalcChunkTile(TILE_W) == 1);

    assert(CalcChunkTile(-1) == CHUNK_W - 1);
    assert(CalcChunkTile(-(CHUNK_W * TILE_W - 1)) == 0);
    assert(CalcChunkTile(-(CHUNK_W * TILE_W)) == 0);
    assert(CalcChunkTile(-(CHUNK_W * TILE_W + 1)) == CHUNK_W - 1);

    const int chunkX = CalcChunk(x);
    const int chunkY = CalcChunk(y);
    const int tileX = CalcChunkTile(x);
    const int tileY = CalcChunkTile(y);
    DLB_ASSERT(tileX >= 0);
    DLB_ASSERT(tileY >= 0);
    DLB_ASSERT(tileX < CHUNK_W);
    DLB_ASSERT(tileY < CHUNK_H);

    auto iter = chunksIndex.find(Chunk::Hash(chunkX, chunkY));
    if (iter != chunksIndex.end()) {
        size_t chunkIdx = iter->second;
        assert(chunkIdx < chunks.size());
        const Chunk &chunk = chunks[chunkIdx];
        size_t tileIdx = tileY * CHUNK_W + tileX;
        assert(tileIdx < ARRAY_SIZE(chunk.tiles));
        return &chunk.tiles[tileY * CHUNK_W + tileX];
    }
    return 0;
}

Chunk &Tilemap::FindOrGenChunk(int64_t seed, int16_t chunkX, int16_t chunkY)
{
    thread_local OpenSimplexEnv *ose = 0;
    if (!ose) ose = initOpenSimplex();
    thread_local OpenSimplexGradients *osg = 0;
    if (!osg) osg = newOpenSimplexGradients(ose, seed);
    thread_local OpenSimplexGradients *osg2 = 0;
    if (!osg2) osg2 = newOpenSimplexGradients(ose, seed + 1);

    ChunkHash chunkHash = Chunk::Hash(chunkX, chunkY);
    auto chunkIter = chunksIndex.find(chunkHash);
    if (chunkIter != chunksIndex.end()) {
        size_t chunkIdx = chunkIter->second;
        assert(chunkIdx < chunks.size());
        Chunk &chunk = chunks[chunkIdx];
        return chunk;
    }

    TraceLog(LOG_INFO, "Generating world chunk [%hd, %hd]", chunkX, chunkY);

    Chunk &chunk = chunks.emplace_back();
    chunksIndex[chunkHash] = chunks.size() - 1;

    chunk.x = chunkX;
    chunk.y = chunkY;

    size_t tileCount = 0;
    for (int tileY = 0; tileY < CHUNK_H; tileY++) {
        for (int tileX = 0; tileX < CHUNK_W; tileX++) {
            int tileIdx = tileY * CHUNK_W + tileX;
            assert(tileIdx < ARRAY_SIZE(chunk.tiles));
            Tile &tile = chunk.tiles[tileY * CHUNK_W + tileX];

            double worldX = (chunk.x * CHUNK_W + tileX) * TILE_W;
            double worldY = (chunk.y * CHUNK_H + tileY) * TILE_W;
            //worldX -= fmod(worldX, TILE_W);
            //worldY -= fmod(worldY, TILE_W);

            Tile::Type tileType{};
#if 1
#define FREQ_BASE_LAYER 1.0 / 8000
#define FREQ_BASE_NOISE 1.0 / 600
            const double base = 0.5 + noise2(ose, osg, worldX * FREQ_BASE_LAYER, worldY * FREQ_BASE_LAYER) / 2.0;
            const double baseNoise = 0.5 + noise2(ose, osg2, worldX * FREQ_BASE_NOISE, worldY * FREQ_BASE_NOISE) / 2.0;
            if (base < 0.15 || (base < 0.25 && baseNoise < 0.7) || (base < 0.35 && baseNoise < 0.3)) {
                tileType = Tile::Type::Water;
            } else if (base < 0.4 || (base < 0.5 && baseNoise < 0.7) || (base < 0.55 && baseNoise < 0.3)) {
                tileType = Tile::Type::Concrete;
            } else if (base < 0.6 || (base < 0.7 && baseNoise < 0.7) || (base < 0.8 && baseNoise < 0.3)) {
                tileType = Tile::Type::Grass;
            } else if (base < 0.9) {
                tileType = Tile::Type::Forest;
            } else {
                tileType = Tile::Type::Concrete;
                tileType = Tile::Type::Forest;
            }
#else
#define FREQ_BASE_LAYER 1.0 / 10000
#define FREQ_BASE_NOISE 1.0 / 3000
            const double base = 0.5 + noise2(ose, osg, xx * FREQ_BASE_LAYER, yy * FREQ_BASE_LAYER) / 2.0;
            const double baseNoise = 0.5 + noise2(ose, osg2, xx * FREQ_BASE_NOISE, yy * FREQ_BASE_NOISE) / 2.0;
            if (base + baseNoise < 0.8) {
                tileType = Tile::Type::Water;
            } else if (base + baseNoise < 0.86) {
                tileType = Tile::Type::Concrete;
            } else if (base + baseNoise < 1.5) {
                tileType = Tile::Type::Grass;
            } else if (base + baseNoise < 1.8) {
                tileType = Tile::Type::Forest;
            } else {
                tileType = Tile::Type::Concrete;
            }
#endif
#undef PERIOD
#undef FREQ
            tile.type = tileType;
            tileCount++;
        }
    }

    assert(tileCount == ARRAY_SIZE(chunk.tiles));

    // TODO: Update minimap when player moves or chunk changes, without re-generating
    // whole thing; and only the chunk is within the cull rect of the minimap.
    //GenerateMinimap();
    return chunk;
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

Tilemap *MapSystem::Alloc(void)
{
    Tilemap *map = mapsFree;
    if (!map) {
        TraceLog(LOG_ERROR, "Tilemap pool is full.\n");
        return 0;
    }

    mapsFree = map->next;
    mapsActiveCount++;

    return map;
}