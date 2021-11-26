#pragma once
#include <cstdint>

struct Tilemap;

enum StructureType {
    Structure_Vault,
};

struct Structure {
    static void Spawn(Tilemap &map, uint32_t x, uint32_t y);
};