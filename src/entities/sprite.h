#pragma once
#include "direction.h"
#include "entities/facet.h"
#include "spritesheet.h"
#include "raylib/raylib.h"

struct Sprite : public Facet {
    const SpriteDef * spriteDef            {};
    float             scale                {};
    Direction         direction            {};
    size_t            animFrameIdx         {};
    double            lastAnimFrameStarted {};
};

const SpriteAnim  &sprite_anim          (const Sprite &sprite);
const SpriteFrame &sprite_frame         (const Sprite &sprite);
Rectangle          sprite_frame_rect    (const Sprite &sprite);
Rectangle          sprite_world_rect    (const Sprite &sprite, const Vector3 &pos);
Vector3            sprite_top_center    (const Sprite &sprite);
Vector3            sprite_center        (const Sprite &sprite);
void               sprite_set_direction (      Sprite &sprite, Vector2 offset);
void               sprite_update        (      Sprite &sprite, double dt);
bool               sprite_cull_body     (const Sprite &sprite, const struct Body3D &body, Rectangle cullRect);
void               sprite_draw_body     (const Sprite &sprite, const struct Body3D &body, const Color &color);
