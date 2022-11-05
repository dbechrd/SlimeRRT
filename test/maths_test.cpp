#include "tests.h"
#include "../src/maths.h"

static void aabb_test()
{
    AABB aabb{};
    aabb.min = { -2.0f, -5.0f };
    aabb.max = { 3.0f, 6.0f };
    float area = aabb.calcArea();
    DLB_ASSERT(area == 55.0f);

    {
        AABB other{};
        other.min = { 2.0f, 5.0f };
        other.max = { 4.0f, 8.0f };
        AABB aabbUnion = aabb.calcUnion(other);
        DLB_ASSERT(aabbUnion.min.x == -2.0f);
        DLB_ASSERT(aabbUnion.min.y == -5.0f);
        DLB_ASSERT(aabbUnion.max.x == 4.0f);
        DLB_ASSERT(aabbUnion.max.y == 8.0f);
    }

    {
        AABB smaller{};
        smaller.min = { aabb.min.x + 1.0f, aabb.min.y + 1.0f };
        smaller.max = { aabb.max.x - 1.0f, aabb.max.y - 1.0f };
        float areaSmaller = smaller.calcArea();
        DLB_ASSERT(areaSmaller == 27.0f);
        float areaIncrease = aabb.calcAreaIncrease(smaller);
        DLB_ASSERT(areaIncrease == 0.0f);
    }

    {
        AABB bigger{};
        bigger.min = { aabb.min.x - 1.0f, aabb.min.y - 1.0f };
        bigger.max = { aabb.max.x + 1.0f, aabb.max.y + 1.0f };
        float areaBigger = bigger.calcArea();
        DLB_ASSERT(areaBigger == 91.0f);
        float areaIncrease = aabb.calcAreaIncrease(bigger);
        DLB_ASSERT(areaIncrease == 36.0f);
    }
}

void maths_test() {
    aabb_test();
}