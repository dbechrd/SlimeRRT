#pragma once
#include "body.h"
#include "draw_command.h"
#include "item_stack.h"

#define ITEM_WORLD_RADIUS 10

struct ItemWorld : Drawable {
    ItemStack stack     {};
    Body3D    body      {};
    Sprite    sprite    {};
    double    spawnedAt {};
    bool      pickedUp  {};
    ItemWorld *next     {};

    int Depth(void) const;
    bool Cull(const Recti& cullRect) const;
    void Draw(void) const;
};