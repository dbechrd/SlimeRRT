#pragma once
#include "body.h"
#include "draw_command.h"

#define ITEM_WORLD_RADIUS 10.0f

struct ItemWorld : Drawable {
    uint32_t  itemUid           {};
    Body3D    body              {};
    Sprite    sprite            {};
    double    spawnedAt         {};
    double    pickedUpAt        {};
    uint32_t  droppedByPlayerId {};
    ItemWorld *next             {};

    Vector3 WorldCenter(void) const;
    Vector3 WorldTopCenter(void) const;
    float Depth(void) const;
    bool Cull(const Rectangle& cullRect) const;
    void Update(double dt);
    void Draw(World &world);
};