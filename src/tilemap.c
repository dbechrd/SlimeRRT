#include "tilemap.h"
#include "dlb_rand.h"
#include "maths.h"
#include "raylib.h"
#include <assert.h>
#include <float.h>
#include <stdlib.h>

static void rrt_build(Tilemap *map, Vector2 qinit, size_t numVertices, float maxGrowthDist);
static size_t rrt_nearest_idx(Tilemap *map, Vector2 p);

bool tile_is_walkable(const Tile *tile)
{
    return tile && tile->tileType != TileType_Water;
}

void tilemap_generate(Tilemap *map)
{
    assert(map);
    assert(map->widthTiles);
    assert(map->heightTiles);
    assert(map->tileset);
    assert(map->tileset->tileWidth);
    assert(map->tileset->tileHeight);
    assert(map->tileset->tileCount);

    map->tileCount = map->widthTiles * map->heightTiles;
    map->tiles = calloc(map->widthTiles * map->heightTiles, sizeof(*map->tiles));

    const size_t tileWidth = map->tileset->tileWidth;
    const size_t tileHeight = map->tileset->tileHeight;

    // NOTE: These parameters are chosen somewhat arbitrarily at this point
    const Vector2 middle = v2_init(map->widthTiles * tileWidth / 2.0f, map->heightTiles * tileHeight / 2.0f);
    const size_t rrtSamples = 512;
    const float maxGrowthDistance = map->widthTiles * tileWidth / 4.0f;
    rrt_build(map, middle, rrtSamples, maxGrowthDistance);

    const Vector2 tileCenterOffset = v2_init(tileWidth / 2.0f, tileHeight / 2.0f);
    for (int y = 0; y < map->heightTiles; y++) {
        for (int x = 0; x < map->widthTiles; x++) {
            //tile.tileType = randomType(g_mersenne);
            //tile.tileType = (tileY * w + tileX) % tileTypeCount; // Debug: Use every tile sequentially
            const Vector2 position = v2_init((float)x * tileWidth, (float)y * tileHeight);
            const Vector2 center = v2_add(position, tileCenterOffset);
            const size_t nearestIdx = rrt_nearest_idx(map, center);

            Tile *tile = tilemap_at(map, x, y);
            //tile->sprite.setPosition(pos);
            //tile->sprite.setTexture(texture);
            tile->tileType = map->rrt.vertices[nearestIdx].tileType;
            tile->position = position;
        }
    }

    // Pass 2: Surround water with sand for @rusteel
    for (int y = 0; y < map->heightTiles; y++) {
        for (int x = 0; x < map->widthTiles; x++) {
            TileType tileType = tilemap_at(map, x, y)->tileType;
            if (tileType == TileType_Water) {
#if 1
                static const int beachWidth = 2;
                for (int beachX = x-beachWidth; beachX <= x+beachWidth; beachX++) {
                    for (int beachY = y-beachWidth; beachY <= y+beachWidth; beachY++) {
                        if (beachX == 0 && beachY == 0) continue;  // this is the water tile
                        Tile *tile = tilemap_at_try(map, beachX, beachY);
                        if (tile && tile->tileType != TileType_Water && tile->tileType != TileType_Concrete) {
                            tile->tileType = TileType_Concrete;
                        }
                    }
                }
#else
                Tile *up    = AtTry(tileX, tileY - 1);
                Tile *down  = AtTry(tileX, tileY + 1);
                Tile *left  = AtTry(tileX - 1, tileY);
                Tile *right = AtTry(tileX + 1, tileY);
                if (up && up->tileType != TileType_Water) {
                    SetTileType(tileX, tileY - 1, TileType_Concrete);
                }
                if (down && down->tileType != TileType_Water) {
                    SetTileType(tileX, tileY + 1, TileType_Concrete);
                }
                if (left && left->tileType != TileType_Water) {
                    SetTileType(tileX - 1, tileY, TileType_Concrete);
                }
                if (right && right->tileType != TileType_Water) {
                    SetTileType(tileX + 1, tileY, TileType_Concrete);
                }
#endif
            }
        }
    }
}

void tilemap_generate_ex(Tilemap *map, size_t width, size_t height, Tileset *tileset)
{
    assert(map);
    assert(width);
    assert(height);
    assert(tileset);

    map->widthTiles = width;
    map->heightTiles = height;
    map->tileset = tileset;
    tilemap_generate(map);
}

void tilemap_free(Tilemap *map)
{
    free(map->tiles);
}

Tile *tilemap_at(Tilemap *map, int tileX, int tileY)
{
    size_t idx = (size_t)tileY * map->widthTiles + tileX;
    assert(idx < (size_t)map->widthTiles * map->heightTiles);
    return &map->tiles[idx];
}

Tile *tilemap_at_try(Tilemap *map, int tileX, int tileY)
{
    Tile *tile = NULL;
    if (tileX >= 0 && tileY >= 0 && tileX < map->widthTiles && tileY < map->widthTiles) {
        size_t idx = (size_t)tileY * map->widthTiles + tileX;
        assert(idx < (size_t)map->widthTiles * map->heightTiles);
        tile = &map->tiles[idx];
    }
    return tile;
}

Tile *tilemap_at_world(Tilemap *map, int x, int y)
{
    assert(x >= 0);
    assert(y >= 0);
    assert(x < (int)map->tileset->tileWidth * map->widthTiles);
    assert(y < (int)map->tileset->tileHeight * map->heightTiles);
    int tileX = x / (int)map->tileset->tileWidth;
    int tileY = y / (int)map->tileset->tileHeight;
    return tilemap_at(map, tileX, tileY);
}

Tile *tilemap_at_world_try(Tilemap *map, int x, int y)
{
    if (x < 0 || y < 0 ||
        (size_t)x >= map->tileset->tileWidth * map->widthTiles ||
        (size_t)y >= map->tileset->tileHeight * map->heightTiles)
    {
        return 0;
    }

    int tileX = x / (int)map->tileset->tileWidth;
    int tileY = y / (int)map->tileset->tileHeight;
    return tilemap_at_try(map, tileX, tileY);
}

static void rrt_build(Tilemap *map, Vector2 qinit, size_t numVertices, float maxGrowthDist)
{
    float maxGrowthDistSq = maxGrowthDist * maxGrowthDist;

    map->rrt.vertexCount = numVertices;
    map->rrt.vertices = calloc(map->rrt.vertexCount, sizeof(*map->rrt.vertices));
    assert(map->rrt.vertices);

    const int tileMin = 0;
    const int tileMax = (int)map->tileset->tileCount - 2;

    RRTVertex *vertex = map->rrt.vertices;
    vertex->tileType = dlb_rand_int(tileMin, tileMax);
    vertex->position = qinit;
    vertex++;

    size_t randTiles = numVertices / 8;

    Vector2 qrand = { 0 };
    for (size_t tileIdx = 1; tileIdx < map->rrt.vertexCount; tileIdx++) {
#if 1
        qrand.x = (float)dlb_rand_int(0, (int)(map->widthTiles * map->tileset->tileWidth));
        qrand.y = (float)dlb_rand_int(0, (int)(map->heightTiles * map->tileset->tileHeight));
#else
        // TODO: Try tile coords instead of pixels
        qrand.tileX = (float)GetRandomValue(0, map->widthTiles);
        qrand.tileY = (float)GetRandomValue(0, map->heightTiles);
#endif
        size_t nearestIdx = rrt_nearest_idx(map, qrand);
        Vector2 qnear = map->rrt.vertices[nearestIdx].position;
        Vector2 qnew = v2_sub(qrand, qnear);
#if 1
        //bool constantIncrement = false;  // always increment by "maxGrowthDist"
        //bool clampIncrement = true;     // increment at most by maxGrowthDist
        //if (constantIncrement || (clampIncrement && Vector2::LengthSq(qnear) > maxGrowthDistSq)) {
        //    qnew = Vector2::Normalize(qrand - qnear) * maxGrowthDist;
        //}
#endif
        qnew = v2_add(qnew, qnear);
        if (tileIdx < randTiles) {
            vertex->tileType = dlb_rand_int(tileMin, tileMax);
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
    size_t minIdx;
    for (size_t i = 0; i < map->rrt.vertexCount; i++) {
        float distSq = v2_distance_sq(p, map->rrt.vertices[i].position);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            minIdx = i;
        }
    }
    return minIdx;
}

////////////////////////////////////////////////////////////////////////////////

#if 0
void SetTileType(Tilemap *map, int tileX, int tileY, TileType tileType)
{
    Tile *tile = tilemap_at(map, tileX, tileY);
    tile->textureRect
    source.tileX = (float)(tilemap.tiles[i].tileType % tilesPerRow * tilemap.tileset->tileWidth);
    source.tileY = (float)(tilemap.tiles[i].tileType / tilesPerRow * tilemap.tileset->tileHeight);
    source.width  = (float)tilemap.tileset->tileWidth;
    source.height = (float)tilemap.tileset->tileHeight;

    Tile *tile = tilemap_at(map, tileX, tileY);
    tile.tileType = tileType;
    sf::Vector2u texSize = texture.getSize();
    int texTilesPerRow = (int)texSize.tileX / tileWidth;
    auto texRect = sf::IntRect(
        (int)tile.tileType % texTilesPerRow * tileWidth,
        (int)tile.tileType / texTilesPerRow * tileHeight,
        tileWidth,
        tileHeight
    );
    tile.sprite.setTextureRect(texRect);
}

void Draw(sf::RenderTarget &target, const sf::Vector2 &viewSize)
{
    mapBatch.setRenderTarget(target);
    const size_t tileCount = (size_t)widthTiles * heightTiles;
    for (size_t i = 0; i < tileCount; i++) {
        mapBatch.draw(map[i].sprite);
    }
    mapBatch.display();
}

void DrawRRT(sf::RenderTarget &target) const
{
    // TODO: Draw this as a debug overlay using geometry shader to create the circles
    sf::RenderStates states = sf::RenderStates::Default;
    size_t vertexCount = rrt.size();
    for (size_t i = 0; i < vertexCount; i++) {
        sf::CircleShape circle;
        circle.setPosition(rrt[i].position);
        circle.setFillColor(sf::Color::Blue);
        circle.setRadius(20.0f);
        target.draw(circle);
    }
}
#endif