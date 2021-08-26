#include "tests.h"
#include "../src/maths.h"
#include <cassert>

static void aabb_test()
{
    AABB aabb{};
    aabb.min = { -2.0f, -5.0f };
    aabb.max = { 3.0f, 6.0f };
    float area = aabb.calcArea();
    assert(area == 55.0f);

    {
        AABB other{};
        other.min = { 2.0f, 5.0f };
        other.max = { 4.0f, 8.0f };
        AABB aabbUnion = aabb.calcUnion(other);
        assert(aabbUnion.min.x == -2.0f);
        assert(aabbUnion.min.y == -5.0f);
        assert(aabbUnion.max.x == 4.0f);
        assert(aabbUnion.max.y == 8.0f);
    }

    {
        AABB smaller{};
        smaller.min = { aabb.min.x + 1.0f, aabb.min.y + 1.0f };
        smaller.max = { aabb.max.x - 1.0f, aabb.max.y - 1.0f };
        float areaSmaller = smaller.calcArea();
        assert(areaSmaller == 27.0f);
        float areaIncrease = aabb.calcAreaIncrease(smaller);
        assert(areaIncrease == 0.0f);
    }

    {
        AABB bigger{};
        bigger.min = { aabb.min.x - 1.0f, aabb.min.y - 1.0f };
        bigger.max = { aabb.max.x + 1.0f, aabb.max.y + 1.0f };
        float areaBigger = bigger.calcArea();
        assert(areaBigger == 91.0f);
        float areaIncrease = aabb.calcAreaIncrease(bigger);
        assert(areaIncrease == 36.0f);
    }
}

void maths_test() {
    aabb_test();
}