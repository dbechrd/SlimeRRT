#pragma once
#include <cstdint>

typedef uint8_t TileType;

enum : TileType {
    TileType_Void,
    TileType_Grass,
    TileType_Water,
    TileType_Forest,
    TileType_Wood,
    TileType_Concrete,
    TileType_Grass2,
    TileType_Grass3,
    TileType_Count
};

struct Tile {
    TileType type      {};
    uint8_t  base      {};
    uint8_t  baseNoise {};

    const char *ToString() const {
        switch (type) {
            case TileType_Void     : return "Void";
            case TileType_Grass    : return "Grass";
            case TileType_Water    : return "Water";
            case TileType_Forest   : return "Forest";
            case TileType_Wood     : return "Wood";
            case TileType_Concrete : return "Concrete";
            case TileType_Grass2   : return "Grass2";
            case TileType_Grass3   : return "Grass3";
        }
    }

    inline bool IsWalkable() const {
        return true; //tileType != TileType::Water;
    }

    inline bool IsSwimmable() const {
        return type == TileType_Water;
    }
};
