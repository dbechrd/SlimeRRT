#include "tilemap.h"
#include "dlb_rand.h"
#include "dlb_types.h"
#include "maths.h"
#include "net_message.h"
#include "raylib/raylib.h"
#include <float.h>
#include <stdlib.h>

void Tilemap::GenerateMinimap(Vector2 worldPos)
{
    // Check for OpenGL context
    if (!IsWindowReady()) {
        return;
    }

    thread_local static int pixelCount{};
    thread_local static Image minimapImg{};
    thread_local static Color tileColors[TileType_Count]{};

    if (!minimap.width) {
        minimapImg.width = CHUNK_W * 8;
        minimapImg.height = CHUNK_W * 8;
        minimapImg.mipmaps = 1;
        minimapImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        pixelCount = minimapImg.width * minimapImg.height;
        DLB_ASSERT(sizeof(Color) == 4);
        minimapImg.data = calloc(pixelCount, sizeof(Color));
        DLB_ASSERT(minimapImg.data);

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
                DLB_ASSERT(chunkIdx < chunks.size());
                Chunk &chunk = chunks[chunkIdx];
                for (int tileY = 0; tileY < CHUNK_W; tileY++) {
                    for (int tileX = 0; tileX < CHUNK_W; tileX++) {
                        Tile &tile = chunk.tiles[tileY * CHUNK_W + tileX];
                        DLB_ASSERT((int)tile.type >= 0);
                        DLB_ASSERT((int)tile.type < (int)TileType_Count);
                        int pixelY = ((chunkY - originChunkY + 4) * CHUNK_W + tileY);
                        int pixelX = ((chunkX - originChunkX + 4) * CHUNK_W + tileX);
                        int pixelIdx = pixelY * minimapImg.width + pixelX;
                        DLB_ASSERT(pixelIdx >= 0);
                        DLB_ASSERT(pixelIdx < pixelCount);
                        minimapPixels[pixelIdx] = tileColors[(int)tile.type];
                    }
                }
            }
        }
    }

    UpdateTexture(minimap, minimapImg.data);
}

int16_t Tilemap::CalcChunk(float world) const
{
    int16_t chunk = (int16_t)floorf(world / (CHUNK_W * TILE_W));
    return chunk;
}

int16_t Tilemap::CalcChunkTile(float world) const
{
    const float chunk = CalcChunk(world);
    const float chunkStart = chunk * CHUNK_W * TILE_W;
    const float chunkOffset = world - chunkStart;

    const float tilef = floorf(chunkOffset / TILE_W);

    int16_t tile = (int16_t)tilef;
    // Sometimes for very small floats precision loss causes tile == CHUNK_W, which is out of range
    tile -= (tile == CHUNK_W);
    DLB_ASSERT(tile >= 0);
    DLB_ASSERT(tile < CHUNK_W);
    return (int16_t)tile;
}

Tile *Tilemap::TileAtWorld(float x, float y)
{
    const int chunkX = CalcChunk(x);
    const int chunkY = CalcChunk(y);
    const int tileX = CalcChunkTile(x);
    const int tileY = CalcChunkTile(y);

#if 1
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

    Tile *tile = 0;
    auto iter = chunksIndex.find(Chunk::Hash(chunkX, chunkY));
    if (iter != chunksIndex.end()) {
        size_t chunkIdx = iter->second;
        DLB_ASSERT(chunkIdx < chunks.size());
        Chunk &chunk = chunks[chunkIdx];
        size_t tileIdx = tileY * CHUNK_W + tileX;
        DLB_ASSERT(tileIdx < ARRAY_SIZE(chunk.tiles));
        tile = &chunk.tiles[tileY * CHUNK_W + tileX];
    }
    return tile;
}

Vector2 Tilemap::TileCenter(Vector2 world) const
{
    Vector2 tileCenter{};
    tileCenter.x = floorf(world.x / TILE_W) * TILE_W + (TILE_W * 0.5f);
    tileCenter.y = floorf(world.y / TILE_W) * TILE_W + (TILE_W * 0.5f);
    return tileCenter;
}

Chunk &Tilemap::FindOrGenChunk(World &world, int16_t chunkX, int16_t chunkY)
{
    ChunkHash chunkHash = Chunk::Hash(chunkX, chunkY);
    auto chunkIter = chunksIndex.find(chunkHash);
    if (chunkIter != chunksIndex.end()) {
        size_t chunkIdx = chunkIter->second;
        DLB_ASSERT(chunkIdx < chunks.size());
        Chunk &chunk = chunks[chunkIdx];
        return chunk;
    }

    //E_INFO("Generating world chunk [%hd, %hd]", chunkX, chunkY);

    Chunk &chunk = chunks.emplace_back();
    chunk.x = chunkX;
    chunk.y = chunkY;
    chunksIndex[chunkHash] = chunks.size() - 1;

    constexpr double FREQ_ELEVATION                             = 1.0 / 16000;
    constexpr double FREQ_ROADS                                 = 1.0 / 4000;
    constexpr double FREQ_ROADS_NOISE                           = 1.0 / 400;
    constexpr double FREQ_ISLAND                                = 1.0 / 3000;
    constexpr double FREQ_BEACH_FALLOFF                         = 1.0 / 400;
    constexpr double FREQ_MEADOW_DETAILS                        = 1.0 / 100;
    constexpr double FREQ_FOREST_MEADOW_FALLOFF                 = 1.0 / 800;
    constexpr double FREQ_FOREST_TREES                          = 1.0 / 200;
    constexpr double FREQ_FOREST_MOUNTAINTOP_LAKE_BEACH_FALLOFF = 1.0 / 300;
    constexpr double FREQ_MOUNTAINTOP_LAKE_FALLOFF              = 1.0 / 600;

#define NOISE_BETWEEN(noise, a, b) ((noise) >= (a) && (noise) < (b))

    size_t tileCount = 0;
    for (int tileY = 0; tileY < CHUNK_H; tileY++) {
        for (int tileX = 0; tileX < CHUNK_W; tileX++) {
            int tileIdx = tileY * CHUNK_W + tileX;
            DLB_ASSERT(tileIdx < (int)ARRAY_SIZE(chunk.tiles));

            // World coords, in pixels
            float x = (float)((chunk.x * CHUNK_W + tileX) * TILE_W);
            float y = (float)((chunk.y * CHUNK_H + tileY) * TILE_W);

            TileType tileType{};
            Object tileObject{};

            const double elev = g_noise.Seq1(x, y, FREQ_ELEVATION);
            if (elev < 0.2) {
                // Lake
                tileType = TileType_Water;

                // Island / sandbar
                const double islandNoise = g_noise.Seq1(x, y, FREQ_ISLAND);
                if (islandNoise >= 0.8) {
                    tileType = TileType_Concrete;
                }
            } else if (elev < 0.23) {
                // Beach
                tileType = TileType_Concrete;

                // Beach fall-off into grass
                if (elev >= 0.22) {
                    const double falloffNoise = g_noise.Seq1(x, y, FREQ_BEACH_FALLOFF);
                    if (falloffNoise >= 0.8) {
                        tileType = TileType_Grass;
                    }
                }
            } else if (elev < 0.6) {
                // Meadow
                tileType = TileType_Grass;

                // Meadow details (decorations/objects)
                if (tileType == TileType_Grass) {
                    const double decoNoise = g_noise.Seq1(x, y, FREQ_MEADOW_DETAILS);
                    if (NOISE_BETWEEN(decoNoise, 0, 0.05)) {
                        // Meadow flowers
                        tileType = TileType_Flowers;
                    //} else if (NOISE_BETWEEN(decoNoise, 0.05, 0.051)) {
                    } else if (NOISE_BETWEEN(decoNoise, 0.05, 0.06)) {
                        // Meadow stones
                        tileObject.type = ObjectType_Rock01;
                        tileObject.SetFlag(ObjectFlag_Collide | ObjectFlag_Interact);
                    }
                }
            } else if (elev < 0.92) {
                // Forest
                tileType = TileType_Forest;

                // Forest fall-off into Meadow
                if (elev < 0.63) {
                    const double alpha = (elev - 0.6) / (0.63 - 0.6);
                    const double falloffNoise = g_noise.Seq1(x, y, FREQ_FOREST_MEADOW_FALLOFF);
                    if (falloffNoise >= alpha) {
                        tileType = TileType_Grass;
                    }
                }

                // Forest trees
                if (tileType == TileType_Forest) {
                    const double alpha = (elev - 0.6) / (0.92 - 0.6);
                    const double falloffNoise = g_noise.Seq1(x, y, FREQ_FOREST_TREES);
                    if (falloffNoise >= fabs(alpha - 0.5) * 2.0) {
                        tileType = TileType_Tree;
                    }
                }

                // Forest fall-off into Mountaintop Lake Beach
                if (elev >= 0.91) {
                    const double alpha = (elev - 0.91) / (0.92 - 0.91);
                    const double falloffNoise = g_noise.Seq1(x, y, FREQ_FOREST_MOUNTAINTOP_LAKE_BEACH_FALLOFF);
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
                    const double falloffNoise = g_noise.Seq1(x, y, FREQ_MOUNTAINTOP_LAKE_FALLOFF);
                    if (falloffNoise >= alpha) {
                        tileType = TileType_Concrete;
                    }
                }

                // Mountaintop Lake treasure
                if (elev > 0.99) {
                    // TODO: Generate a semi-hidden treasure structure instead? These items will despawn.
                    world.itemSystem.SpawnItem({ x, y, 0 }, ItemType_Orig_Gem_GoldenChest, 1);
                }
            }

            if (elev >= 0.3 && elev <= 0.32) {
                tileType = TileType_Wood;
            }

            switch (tileType) {
                case TileType_Grass: case TileType_Forest: case TileType_Flowers: case TileType_Tree: {
                    const double road = g_noise.Seq1(x, y, FREQ_ROADS);
                    if (road >= 0.5 && road <= 0.55) {
                        const double roadNoise = g_noise.Seq1(x, y, FREQ_ROADS_NOISE);
                        if (NOISE_BETWEEN(roadNoise, 0.15, 0.17) ||
                            NOISE_BETWEEN(roadNoise, 0.20, 0.28) ||
                            NOISE_BETWEEN(roadNoise, 0.31, 0.37) ||
                            NOISE_BETWEEN(roadNoise, 0.45, 0.47) ||
                            NOISE_BETWEEN(roadNoise, 0.52, 0.55) ||
                            NOISE_BETWEEN(roadNoise, 0.70, 0.73)) {
                        } else {
                            tileType = TileType_Dirt;
                            tileObject = {};
                        }
                        break;
                    }
                    break;
                }
                default: break;
            }

            Tile &tile = chunk.tiles[tileY * CHUNK_W + tileX];
            tile.type = tileType;
            tile.object = tileObject;
            tileCount++;
        }
    }

#undef NOISE_BETWEEN

    DLB_ASSERT(tileCount == ARRAY_SIZE(chunk.tiles));

    // TODO: Update minimap when player moves or chunk changes, without re-generating
    // whole thing; and only the chunk is within the cull rect of the minimap.
    //GenerateMinimap();
    return chunk;
}

MapSystem::~MapSystem(void)
{
    for (Tilemap &map : maps) {
        UnloadTexture(map.minimap);
    }
}

Tilemap &MapSystem::Alloc()
{
    Tilemap &map = maps.emplace_back();
    return map;
}