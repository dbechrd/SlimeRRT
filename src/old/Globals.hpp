#pragma once

#define PI 3.14159265358979323846264338327950288

#define ARRAY_COUNT(x) (sizeof(x)/sizeof(*x))

#include <random>
extern std::random_device g_randDevice;
extern std::mt19937 g_mersenne;