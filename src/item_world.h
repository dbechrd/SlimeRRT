#pragma once
#include "body.h"
#include "draw_command.h"
#include "item_stack.h"

#define ITEM_WORLD_RADIUS 10.0f

struct ItemWorld : Drawable {
    uint32_t  id         {};
    ItemStack stack      {};
    Body3D    body       {};
    Sprite    sprite     {};
    double    spawnedAt  {};
    double    pickedUpAt {};
    ItemWorld *next      {};

    float Depth(void) const;
    bool Cull(const Rectangle& cullRect) const;
    void Draw(void) const;
};