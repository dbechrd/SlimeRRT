#pragma once
#include "raylib.h"

#define SQUARED(x) ((x)*(x))

// Generate random number in [0.0f, 1.0f] range, with specified resolution (higher = smaller increments in variability)
static inline float random_normalized(int resolution)
{
    float value = GetRandomValue(0, resolution) / (float)resolution;
    return value;
}

// Generate random number in [-1.0f, 1.0f] range, with specified resolution (higher = smaller increments in variability)
static inline float random_normalized_signed(int resolution)
{
    float value = GetRandomValue(0, resolution) / (float)resolution * 2.0f - 1.0f;
    return value;
}