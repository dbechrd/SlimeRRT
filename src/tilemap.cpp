#include "tilemap.h"
#include "dlb_rand.h"
#include "dlb_types.h"
#include "maths.h"
#include "net_message.h"
#include "raylib/raylib.h"
#include <cassert>
#include <float.h>
#include <stdlib.h>

Tilemap::~Tilemap(void)
{
    //free(tiles);
    free(rrt.vertices);
    UnloadTexture(minimap);
    FreeSimplex();
}

void Tilemap::SeedSimplex(int64_t seed)
{
    // NOTE(dlb): Don't free/alloc the ose every tilemap. Not necessary, should be global.
    // Only need to free/alloc the gradients when seed is different.
    FreeSimplex();
    ose = initOpenSimplex();
    osg = initOpenSimplexGradients(ose, seed);
    osg2 = initOpenSimplexGradients(ose, seed + 1);
}

void Tilemap::FreeSimplex(void)
{
    freeOpenSimplex(ose);
    freeOpenSimplexGradients(osg);
    freeOpenSimplexGradients(osg2);
}

bool Tilemap::GenerateNoise(Texture &tex)
{
    // Check for OpenGL context
    if (!IsWindowReady()) {
        return false;
    }

    OpenSimplexEnv *noise_ose = initOpenSimplex();
    OpenSimplexGradients *noise_osg = initOpenSimplexGradients(ose, 1234);

#define WIDTH 256 //4096
#define HEIGHT 256 //4096
#define PERIOD 64.0
#define OFF_X 2048
#define OFF_Y 2048
#define FREQ 1.0 / PERIOD

    Image noiseImg{};
    noiseImg.width = WIDTH;
    noiseImg.height = HEIGHT;
    noiseImg.mipmaps = 1;
    noiseImg.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE; //PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    noiseImg.data = calloc(WIDTH * HEIGHT, sizeof(uint8_t));
    assert(noiseImg.data);

    uint8_t *noisePixel = (uint8_t *)noiseImg.data;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            const double noise = noise2(ose, osg, (x + OFF_X) * FREQ, (y + OFF_Y) * FREQ);
            //printf("%0.2f ", noise);

            *noisePixel = (uint8_t)(128.0 + noise * 128.0); // tileColors[(int)tile.type];
            //printf("%d ", *minimapPixel);
            noisePixel++;
        }
        //printf("\n");
    }

#undef WIDTH
#undef HEIGHT
#undef PERIOD
#undef OFF_X
#undef OFF_Y
#undef FREQ

    tex = LoadTextureFromImage(noiseImg);
    free(noiseImg.data);
    freeOpenSimplex(noise_ose);
    freeOpenSimplexGradients(noise_osg);
    return true;
}

void Tilemap::GenerateMinimap(Vector2 worldPos)
{
    // Check for OpenGL context
    if (!IsWindowReady()) {
        return;
    }

    thread_local int pixelCount{};
    thread_local Image minimapImg{};
    thread_local Color tileColors[TileType_Count]{};

    if (!minimap.width) {
        minimapImg.width = CHUNK_W * 8;
        minimapImg.height = CHUNK_W * 8;
        minimapImg.mipmaps = 1;
        minimapImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        pixelCount = minimapImg.width * minimapImg.height;
        assert(sizeof(Color) == 4);
        minimapImg.data = calloc(pixelCount, sizeof(Color));
        assert(minimapImg.data);

        tileColors[(int)TileType_Void] = BLACK;
        tileColors[(int)TileType_Grass] = GREEN;
        tileColors[(int)TileType_Water] = SKYBLUE;
        tileColors[(int)TileType_Forest] = DARKGREEN;
        tileColors[(int)TileType_Wood] = BROWN;
        tileColors[(int)TileType_Concrete] = GRAY;
        tileColors[(int)TileType_Grass2] = GREEN;
        tileColors[(int)TileType_Grass3] = GREEN;

        minimap = LoadTextureFromImage(minimapImg);
    }

    // NOTE: This is the client-side world map. Fog of war until tile types known?
    Color *minimapPixels = (Color *)minimapImg.data;
    memset(minimapPixels, 0, pixelCount * sizeof(*minimapPixels));

    const int originChunkX = CalcChunk(worldPos.x);
    const int originChunkY = CalcChunk(worldPos.y);
    for (int chunkY = originChunkY - 4; chunkY < originChunkY + 4; chunkY++) {
        for (int chunkX = originChunkX - 4; chunkX < originChunkX + 4; chunkX++) {
            ChunkHash chunkHash = Chunk::Hash(chunkX, chunkY);
            auto chunkIter = chunksIndex.find(chunkHash);
            if (chunkIter != chunksIndex.end()) {
                size_t chunkIdx = chunkIter->second;
                assert(chunkIdx < chunks.size());
                Chunk &chunk = chunks[chunkIdx];
                for (int tileY = 0; tileY < CHUNK_W; tileY++) {
                    for (int tileX = 0; tileX < CHUNK_W; tileX++) {
                        Tile &tile = chunk.tiles[tileY * CHUNK_W + tileX];
                        assert((int)tile.type >= 0);
                        assert((int)tile.type < (int)TileType_Count);
                        int pixelY = ((chunkY - originChunkY + 4) * CHUNK_W + tileY);
                        int pixelX = ((chunkX - originChunkX + 4) * CHUNK_W + tileX);
                        int pixelIdx = pixelY * minimapImg.width + pixelX;
                        assert(pixelIdx >= 0);
                        assert(pixelIdx < pixelCount);
                        minimapPixels[pixelIdx] = tileColors[(int)tile.type];
                    }
                }
            }
        }
    }

    UpdateTexture(minimap, minimapImg.data);
}

//const int16_t Tilemap::CalcChunk(float world) const
//{
//    const float chunk = world / CHUNK_W / TILE_W;
//    const float chunkFixNeg = chunk < 0.0f ? ceilf(chunk) - 1 : floorf(chunk);
//    return (int16_t)chunkFixNeg;
//}
//
//const int16_t Tilemap::CalcChunkTile(float world) const
//{
//    const float chunk = CalcChunk(world);
//    const float chunkStart = chunk * CHUNK_W * TILE_W;
//    const float chunkOffset = world - chunkStart;
//    const float tilef = CLAMP(floorf(chunkOffset / TILE_W), 0, CHUNK_W - 1);
//    int16_t tile = (int16_t)tilef;
//    DLB_ASSERT(tile >= 0);
//    DLB_ASSERT(tile < CHUNK_W);
//    return tile;
//}

const int16_t Tilemap::CalcChunk(float world) const
{
    int16_t chunk = (int16_t)floorf(world / (CHUNK_W * TILE_W));
    return chunk;
}

const int16_t Tilemap::CalcChunkTile(float world) const
{
    const float chunk = CalcChunk(world);
    const float chunkStart = chunk * CHUNK_W * TILE_W;
    const float chunkOffset = world - chunkStart;

    //const float tilef = CLAMP(floorf(chunkOffset / TILE_W), 0, CHUNK_W - 1);
    const float tilef = floorf(chunkOffset / TILE_W);

    int16_t tile = (int16_t)tilef;
    // Sometimes for very small floats precision loss causes tile == CHUNK_W, which is out of range
    if (tile == CHUNK_W) {
        tile--;
    }
    DLB_ASSERT(tile >= 0);
    DLB_ASSERT(tile < CHUNK_W);
    return (int16_t)tile;
}

const Tile *Tilemap::TileAtWorld(float x, float y) const
{
    const int chunkX = CalcChunk(x);
    const int chunkY = CalcChunk(y);
    const int tileX = CalcChunkTile(x);
    const int tileY = CalcChunkTile(y);

#if 0
    // TODO: Move to tests
    DLB_ASSERT(CalcChunk(-(CHUNK_W * TILE_W + 1)) == -2);
    DLB_ASSERT(CalcChunk(-(CHUNK_W * TILE_W)) == -1);
    DLB_ASSERT(CalcChunk(-(CHUNK_W * TILE_W - 1)) == -1);
    DLB_ASSERT(CalcChunk(-1) == -1);
    DLB_ASSERT(CalcChunk(-0.001f) == -1);
    DLB_ASSERT(CalcChunk(0) == 0);
    DLB_ASSERT(CalcChunk(1) == 0);
    DLB_ASSERT(CalcChunk(CHUNK_W * TILE_W - 1) == 0);
    DLB_ASSERT(CalcChunk(CHUNK_W * TILE_W)        == 1);

    DLB_ASSERT(CalcChunkTile(-(CHUNK_W * TILE_W + 1)) == CHUNK_W - 1);
    DLB_ASSERT(CalcChunkTile(-(CHUNK_W * TILE_W)) == 0);
    DLB_ASSERT(CalcChunkTile(-(CHUNK_W * TILE_W - 1)) == 0);
    DLB_ASSERT(CalcChunkTile(-1) == CHUNK_W - 1);
    DLB_ASSERT(CalcChunkTile(-0.000000003f) == CHUNK_W - 1);
    DLB_ASSERT(CalcChunkTile(0) == 0);
    DLB_ASSERT(CalcChunkTile(1) == 0);
    DLB_ASSERT(CalcChunkTile(TILE_W - 1) == 0);
    DLB_ASSERT(CalcChunkTile(TILE_W) == 1);

    DLB_ASSERT(tileX >= 0);
    DLB_ASSERT(tileY >= 0);
    DLB_ASSERT(tileX < CHUNK_W);
    DLB_ASSERT(tileY < CHUNK_H);
#endif

    auto iter = chunksIndex.find(Chunk::Hash(chunkX, chunkY));
    if (iter != chunksIndex.end()) {
        size_t chunkIdx = iter->second;
        DLB_ASSERT(chunkIdx < chunks.size());
        const Chunk &chunk = chunks[chunkIdx];
        size_t tileIdx = tileY * CHUNK_W + tileX;
        DLB_ASSERT(tileIdx < ARRAY_SIZE(chunk.tiles));
        return &chunk.tiles[tileY * CHUNK_W + tileX];
    }
    return 0;
}

Chunk &Tilemap::FindOrGenChunk(World &world, int16_t chunkX, int16_t chunkY)
{
    assert(ose);
    assert(osg);
    assert(osg2);

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

            float worldX = (float)((chunk.x * CHUNK_W + tileX) * TILE_W);
            float worldY = (float)((chunk.y * CHUNK_H + tileY) * TILE_W);
            //worldX -= fmod(worldX, TILE_W);cd Dcd
            //worldY -= fmod(worldY, TILE_W);

            TileType tileType{};
#if 1
#define FREQ_ELEVATION                             1.0 / 16000
#define FREQ_ISLAND                                1.0 / 3000
#define FREQ_BEACH_FALLOFF                         1.0 / 400
#define FREQ_MEADOW_FLOWERS                        1.0 / 100
#define FREQ_FOREST_MEADOW_FALLOFF                 1.0 / 800
#define FREQ_FOREST_TREES                          1.0 / 200
#define FREQ_FOREST_MOUNTAINTOP_LAKE_BEACH_FALLOFF 1.0 / 300
#define FREQ_MOUNTAINTOP_LAKE_FALLOFF              1.0 / 600
#define NOISE_SAMPLE(freq) (0.5 + noise2(ose, osg, worldX * freq, worldY * freq) / 2.0)
            const double elev = NOISE_SAMPLE(FREQ_ELEVATION);
            if (elev < 0.2) {
                // Lake
                tileType = TileType_Water;

                // Island / sandbar
                const double islandNoise = NOISE_SAMPLE(FREQ_ISLAND);
                if (islandNoise >= 0.8) {
                    tileType = TileType_Concrete;
                }
            } else if (elev < 0.23) {
                // Beach
                tileType = TileType_Concrete;

                // Beach fall-off into grass
                if (elev >= 0.22) {
                    const double falloffNoise = NOISE_SAMPLE(FREQ_BEACH_FALLOFF);
                    if (falloffNoise >= 0.8) {
                        tileType = TileType_Grass;
                    }
                }
            } else if (elev < 0.6) {
                // Meadow
                tileType = TileType_Grass;

                // Meadow flowers
                if (tileType == TileType_Grass) {
                    const double falloffNoise = NOISE_SAMPLE(FREQ_MEADOW_FLOWERS);
                    if (falloffNoise >= 0.95) {
                        tileType = TileType_Flowers;
                    }
                }
            } else if (elev < 0.92) {
                // Forest
                tileType = TileType_Forest;

                // Forest fall-off into Meadow
                if (elev < 0.63) {
                    const double alpha = (elev - 0.6) / (0.63 - 0.6);
                    const double falloffNoise = NOISE_SAMPLE(FREQ_FOREST_MEADOW_FALLOFF);
                    if (falloffNoise >= alpha) {
                        tileType = TileType_Grass;
                    }
                }

                // Forest trees
                if (tileType == TileType_Forest) {
                    const double alpha = (elev - 0.6) / (0.92 - 0.6);
                    const double falloffNoise = NOISE_SAMPLE(FREQ_FOREST_TREES);
                    if (falloffNoise >= fabs(alpha - 0.5) * 2.0) {
                        tileType = TileType_Tree;
                    }
                }

                // Forest fall-off into Mountaintop Lake Beach
                if (elev >= 0.91) {
                    const double alpha = (elev - 0.91) / (0.92 - 0.91);
                    const double falloffNoise = NOISE_SAMPLE(FREQ_FOREST_MOUNTAINTOP_LAKE_BEACH_FALLOFF);
                    if (falloffNoise >= 1.0 - alpha) {
                        tileType = TileType_Concrete;
                    }
                }
            } else {
                // Mountaintop Lake
                tileType = TileType_Water;

                // Mountaintop Lake fall-off into Mountaintop Lake Beach
                if (elev < 0.93) {
                    const double alpha = (elev - 0.92) / (0.93 - 0.92);
                    const double falloffNoise = NOISE_SAMPLE(FREQ_MOUNTAINTOP_LAKE_FALLOFF);
                    if (falloffNoise >= alpha) {
                        tileType = TileType_Concrete;
                    }
                }

                // Mountaintop Lake treasure
                if (elev > 0.99) {
                    // TODO: Generate a semi-hidden treasure structure instead? These items will despawn.
                    world.itemSystem.SpawnItem({ worldX, worldY, 0 }, ItemType_Orig_Gem_GoldenChest, 1);
                }
            }

            if (tileType == TileType_Grass) {

            }
#else
#define FREQ_BASE_LAYER 1.0 / 10000
#define FREQ_BASE_NOISE 1.0 / 3000
            const double base = 0.5 + noise2(ose, osg, xx * FREQ_BASE_LAYER, yy * FREQ_BASE_LAYER) / 2.0;
            const double baseNoise = 0.5 + noise2(ose, osg2, xx * FREQ_BASE_NOISE, yy * FREQ_BASE_NOISE) / 2.0;
            if (base + baseNoise < 0.8) {
                tileType = TileType_Water;
            } else if (base + baseNoise < 0.86) {
                tileType = TileType_Concrete;
            } else if (base + baseNoise < 1.5) {
                tileType = TileType_Grass;
            } else if (base + baseNoise < 1.8) {
                tileType = TileType_Forest;
            } else {
                tileType = TileType_Concrete;
            }
#endif
#undef PERIOD
#undef FREQ
            tile.type = tileType;
            tile.base = (uint8_t)(elev * 255);
            tile.baseNoise = 1; //(uint8_t)(baseNoise * 255);
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

Tilemap *MapSystem::Alloc()
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