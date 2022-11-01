#pragma once
#include "body.h"
#include "draw_command.h"

#define ITEM_WORLD_RADIUS 10.0f

typedef uint32_t EntityUID;

struct WorldItem : Drawable {
    EntityUID euid              {};
    ItemStack stack             {};
    Body3D    body              {};
    Sprite    sprite            {};
    double    spawnedAt         {};
    //double    pickedUpAt        {};
    double    despawnedAt       {};
    uint32_t  droppedByPlayerId {};

    Vector3 WorldCenter    (void) const;
    Vector3 WorldTopCenter (void) const;
    void    Update         (double dt);
    float   Depth          (void) const;
    bool    Cull           (const Rectangle& cullRect) const;
    void    Draw           (World &world, Vector2 at) const override;

private:
    ItemStack namedStack {};
    char      name       [ITEM_NAME_MAX_LENGTH]{};
};