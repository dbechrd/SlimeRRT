#pragma once
#include "body.h"
#include "draw_command.h"
#include "item_stack.h"

struct ItemWorld : Drawable {
    ItemStack stack     {};
    Body3D    body      {};
    Color     color     {}; // TODO: sprite
    double    spawnedAt {};
    ItemWorld *next     {};

    float Depth(void) const;
    bool Cull(const Rectangle& cullRect) const;
    void Draw(void) const;
};