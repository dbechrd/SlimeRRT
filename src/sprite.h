#pragma once
#include "direction.h"
#include "spritesheet.h"
#include "raylib/raylib.h"

struct Sprite {
public:
    const SpriteDef * spriteDef            {};
    float             scale                {};
    Direction         direction            {};
    size_t            animFrameIdx         {};
    double            lastAnimFrameStarted {};
};

const SpriteAnim  &sprite_anim             (const Sprite &sprite);
const SpriteFrame &sprite_frame            (const Sprite &sprite);
Rectangle          sprite_frame_rect       (const Sprite &sprite);
Rectangle          sprite_world_rect       (const Sprite &sprite, Vector3 position, float scale);
Vector3            sprite_world_top_center (const Sprite &sprite, Vector3 position, float scale);
Vector3            sprite_world_center     (const Sprite &sprite, Vector3 position, float scale);
void               sprite_update           (      Sprite &sprite, double now, double dt);
bool               sprite_cull_body        (const Sprite &sprite, const struct Body3D &body, Rectangle cullRect);
void               sprite_draw_body        (const Sprite &sprite, const struct Body3D &body, Color color);
