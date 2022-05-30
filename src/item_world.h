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

    Vector3 WorldCenter(void) const;
    Vector3 WorldTopCenter(void) const;
    float Depth(void) const;
    bool Cull(const Rectangle& cullRect) const;
    void Update(double dt);
    void Draw(const World &world) const;

private:
    char name[64]{};
    ItemStack namedStack{};  // so that we know if/when to refresh name buffer
};