#pragma once
#include "body.h"
#include "draw_command.h"
#include "item_stack.h"

#define ITEM_WORLD_RADIUS 10.0f

struct ItemWorld : Drawable {
    ItemStack stack     {};
    Body3D    body      {};
    Color     color     {}; // TODO: sprite
    double    spawnedAt {};
    bool      pickedUp  {};
    ItemWorld *next     {};

    float Depth(void) const;
    bool Cull(const Rectangle& cullRect) const;
    void Draw(void) const;
};