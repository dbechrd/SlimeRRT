#pragma once
#include <cstdint>

struct Tilemap;

enum StructureType {
    Structure_Vault,
};

struct Structure {
    static void Spawn(Tilemap &map, int x, int y);
};