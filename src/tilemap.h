#pragma once
#include "tileset.h"
#include "math.h"
#include "object.h"
#include "dlb_rand.h"
#include "OpenSimplex2F.h"
#include <vector>
#include <unordered_map>

struct World;
typedef uint32_t ChunkHash;

struct Noise {
    void Seed(int64_t seed) {
        ose = initOpenSimplex();
        osg = initOpenSimplexGradients(ose, seed);
    }

    void Free(void)
    {
        freeOpenSimplex(ose);
        freeOpenSimplexGradients(osg);
    }

    double Seq1(double x, double y, double freq)
    {
        assert(ose);
        assert(osg);
        return 0.5 + noise2(ose, osg, x * freq, y * freq) * 0.5;
    }

    double Seq2(double x, double y, double freq)
    {
        assert(ose);
        assert(osg);
        return 0.5 + noise2(ose, osg, (x + 212369) * freq, (y + 314927) * freq) * 0.5;
    }

    bool GenerateTexture(Texture &tex)
    {
        // Check for OpenGL context
        if (!IsWindowReady()) {
            return false;
        }

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
        return true;
    }

private:
    OpenSimplexEnv       * ose {};
    OpenSimplexGradients * osg {};
} g_noise;

struct Chunk {
    int16_t x     {};                     // chunk x offset
    int16_t y     {};                     // chunk y offset
    Tile    tiles [CHUNK_W * CHUNK_W]{};  // 32x32 tiles per chunk

    inline ChunkHash Hash(void) const {
        return ((uint16_t)x << 16) | (uint16_t)y;
    };
    static inline ChunkHash Hash(int16_t x, int16_t y) {
        return ((uint16_t)x << 16) | (uint16_t)y;
    };
};

struct Tilemap {
    Texture            minimap   {};
    TilesetID          tilesetId {};
    std::vector<Chunk> chunks    {};  // TODO: RingBuffer, this set will grow indefinitely
    std::unordered_map<ChunkHash, size_t> chunksIndex{};  // [x << 16 | y] -> idx into chunks array

    ~Tilemap                ();
    void GenerateMinimap    (Vector2 worldPos);
    int16_t CalcChunk       (float world) const;
    int16_t CalcChunkTile   (float world) const;
    Tile *TileAtWorld       (float x, float y);  // Return tile at pixel position in world space, or null
    Vector2 TileCenter      (Vector2 world) const;  // Return tile center in world position
    Chunk &FindOrGenChunk   (World &world, int16_t x, int16_t y);
};

struct MapSystem {
    Tilemap &Alloc();

private:
    std::vector<Tilemap> maps {};
};