#pragma once
#include <cstdint>

struct Tile {
    enum class Type : uint8_t {
        Void,
        Grass,
        Water,
        Forest,
        Wood,
        Concrete,
        Grass2,
        Grass3,
        Count
    };

    Type type {};

    inline bool IsWalkable() const {
        return true; //tileType != TileType::Water;
    }

    inline bool IsSwimmable() const {
        return type == Type::Water;
    }
};
