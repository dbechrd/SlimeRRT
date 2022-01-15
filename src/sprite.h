#pragma once
#include "direction.h"
#include "maths.h"
#include "spritesheet.h"
#include "raylib/raylib.h"

struct Sprite {
public:
    const SpriteDef * spriteDef            {};
    int               scale                {};
    Direction         direction            {};
    size_t            animFrameIdx         {};
    double            lastAnimFrameStarted {};
};

const SpriteAnim  &sprite_anim             (const Sprite &sprite);
const SpriteFrame &sprite_frame            (const Sprite &sprite);
Recti              sprite_frame_rect       (const Sprite &sprite);
Recti              sprite_world_rect       (const Sprite &sprite, const Vector3i &position, int scale);
Vector3i           sprite_world_top_center (const Sprite &sprite, const Vector3i &position, int scale);
Vector3i           sprite_world_center     (const Sprite &sprite, const Vector3i &position, int scale);
void               sprite_update           (      Sprite &sprite, double dt);
bool               sprite_cull_body        (const Sprite &sprite, const struct Body3D &body, const Recti &cullRect);
void               sprite_draw_body        (const Sprite &sprite, const struct Body3D &body, const Color &color);
