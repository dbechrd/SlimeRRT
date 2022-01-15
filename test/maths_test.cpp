#include "tests.h"
#include "../src/maths.h"
#include <cassert>

static void aabb_test()
{
    AABB aabb{};
    aabb.min = { -2, -5 };
    aabb.max = { 3, 6 };
    const int area = aabb.calcArea();
    assert(area == 55);

    {
        AABB other{};
        other.min = { 2, 5 };
        other.max = { 4, 8 };
        AABB aabbUnion = aabb.calcUnion(other);
        assert(aabbUnion.min.x == -2);
        assert(aabbUnion.min.y == -5);
        assert(aabbUnion.max.x == 4);
        assert(aabbUnion.max.y == 8);
    }

    {
        AABB smaller{};
        smaller.min = { aabb.min.x + 1, aabb.min.y + 1 };
        smaller.max = { aabb.max.x - 1, aabb.max.y - 1 };
        const int areaSmaller = smaller.calcArea();
        assert(areaSmaller == 27);
        const int areaIncrease = aabb.calcAreaIncrease(smaller);
        assert(areaIncrease == 0);
    }

    {
        AABB bigger{};
        bigger.min = { aabb.min.x - 1, aabb.min.y - 1 };
        bigger.max = { aabb.max.x + 1, aabb.max.y + 1 };
        const int areaBigger = bigger.calcArea();
        assert(areaBigger == 91);
        const int areaIncrease = aabb.calcAreaIncrease(bigger);
        assert(areaIncrease == 36);
    }
}

void maths_test() {
    aabb_test();
}