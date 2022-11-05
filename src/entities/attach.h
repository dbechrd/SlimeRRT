#pragma once
#include "facet.h"

enum AttachType {
    Attach_Gut,
    Attach_Count
};

struct Attach : public Facet {
    Vector3 points[Attach_Count]{};
};