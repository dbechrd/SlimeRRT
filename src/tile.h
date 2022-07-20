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

    TileType_Structure_TL,
    TileType_Structure_TC,
    TileType_Structure_TR,
    TileType_Tree,
    TileType_Flowers,
    TileType_Unused2,
    TileType_Unused3,
    TileType_Unused4,

    TileType_Structure_ML,
    TileType_Structure_MC,
    TileType_Structure_MR,
    TileType_Unused5,
    TileType_Unused6,
    TileType_Unused7,
    TileType_Unused8,
    TileType_Unused9,

    TileType_Structure_BL,
    TileType_Structure_BC,
    TileType_Structure_BR,
    TileType_Unused10,
    TileType_Unused11,
    TileType_Unused12,
    TileType_Unused13,
    TileType_Unused14,

    TileType_Count
};

struct Tile {
    TileType type{};

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
            default                : return "<TileType>";
        }
    }

    inline bool IsWalkable() const {
        return true; //tileType != TileType::Water;
    }

    inline bool IsSwimmable() const {
        return type == TileType_Water;
    }
};
