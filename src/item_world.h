#pragma once
#include "body.h"
#include "draw_command.h"

#define ITEM_WORLD_RADIUS 10.0f

typedef uint32_t EntityUID;

struct ItemWorld : Drawable {
    EntityUID euid              {};
    ItemStack stack             {};
    Body3D    body              {};
    Sprite    sprite            {};
    double    spawnedAt         {};
    double    pickedUpAt        {};
    uint32_t  droppedByPlayerId {};
    ItemWorld *next             {};

    Vector3 WorldCenter    (void) const;
    Vector3 WorldTopCenter (void) const;
    float   Depth          (void) const;
    bool    Cull           (const Rectangle& cullRect) const;
    void    Update         (double dt);
    void    Draw           (World &world);

private:
    ItemStack namedStack {};
    char      name       [ITEM_NAME_MAX_LENGTH]{};
};