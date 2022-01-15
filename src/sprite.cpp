#include "sprite.h"
#include "spritesheet.h"
#include "body.h"
#include <cassert>

const SpriteAnim &sprite_anim(const Sprite &sprite)
{
    assert(sprite.spriteDef);
    assert(sprite.spriteDef->spritesheet);
    assert(sprite.spriteDef->animations);

    const Spritesheet *sheet = sprite.spriteDef->spritesheet;
    const int animationIdx = sprite.spriteDef->animations[(int)sprite.direction];
    assert(animationIdx >= 0 && animationIdx < sheet->animations.size());
    const SpriteAnim &anim = sheet->animations[animationIdx];
    return anim;
}

const SpriteFrame &sprite_frame(const Sprite &sprite)
{
    if (!sprite.spriteDef) {
        static SpriteFrame frame{};
        frame.x = 0;
        frame.y = 0;
        frame.width = 16;
        frame.height = 16;
        return frame;
    }

    assert(sprite.spriteDef);
    assert(sprite.spriteDef->spritesheet);

    const SpriteAnim &animation = sprite_anim(sprite);
    assert(sprite.animFrameIdx >= 0);
    assert(sprite.animFrameIdx < animation.frameCount);

    const int frameIdx = animation.frames[sprite.animFrameIdx];
    const Spritesheet *sheet = sprite.spriteDef->spritesheet;
    assert(frameIdx >= 0);
    assert(frameIdx < sheet->frames.size());

    const SpriteFrame &frame = sheet->frames[frameIdx];
    return frame;
}

Recti sprite_frame_rect(const Sprite &sprite)
{
    const SpriteFrame &frame = sprite_frame(sprite);
    Recti rect{};
    rect.x = frame.x;
    rect.y = frame.y;
    rect.width  = frame.width;
    rect.height = frame.height;
    return rect;
}

Recti sprite_world_rect(const Sprite &sprite, const Vector3i &position, int scale)
{
    Recti frameRect = sprite_frame_rect(sprite);
    Recti rect = {};
    rect.x = position.x - PIXELS_TO_UNITS(frameRect.width) * scale / 2;
    rect.y = position.y - PIXELS_TO_UNITS(frameRect.height) * scale - position.z;
    rect.width = frameRect.width * scale;
    rect.height = frameRect.height * scale;
    return rect;
}

Vector3i sprite_world_top_center(const Sprite &sprite, const Vector3i &position, int scale)
{
    Recti frameRect = sprite_frame_rect(sprite);
    Vector3i center = {};
    center.x = position.x;
    center.y = position.y;
    center.z = position.z + PIXELS_TO_UNITS(frameRect.height) * scale;
    return center;
}

Vector3i sprite_world_center(const Sprite &sprite, const Vector3i &position, int scale)
{
    Recti frameRect = sprite_frame_rect(sprite);
    Vector3i center = {};
    center.x = position.x;
    center.y = position.y;
    center.z = position.z + PIXELS_TO_UNITS(frameRect.height) * scale / 2;
    return center;
}

void sprite_update(Sprite &sprite, double dt)
{
    UNUSED(dt);
    if (!sprite.spriteDef) {
        return;
    }

    // TODO: SpriteAnim might want to store animation FPS and/or per-frame delays
    const SpriteAnim &anim = sprite_anim(sprite);
    if (anim.frameCount > 1) {
        const double animFps = 24.0;
        const double animDelay = 1.0 / animFps;
        if (glfwGetTime() - sprite.lastAnimFrameStarted > animDelay) {
            sprite.animFrameIdx++;
            sprite.animFrameIdx %= anim.frameCount;
            sprite.lastAnimFrameStarted = glfwGetTime();
        }
    }
}

bool sprite_cull_body(const Sprite &sprite, const Body3D &body, const Recti &cullRect)
{
    const Recti bodyRect = sprite_world_rect(sprite, body.position, sprite.scale);
    bool cull = !CheckCollisionRecti(bodyRect, cullRect);
    return cull;
}

static void sprite_draw(const Sprite &sprite, const Recti &pos, Color color)
{
#if DEMO_BODY_RECT
    // DEBUG: Draw collision Recti
    DrawRectangleRec(dest, Fade(RED, 0.2f));
#endif

    if (sprite.spriteDef) {
        Recti rect = sprite_frame_rect(sprite);

        // Draw textured sprite
        const Rectangle src {
            (float)rect.x,
            (float)rect.y,
            (float)rect.width,
            (float)rect.height
        };
        const Rectangle dst {
            UNITS_TO_PIXELS((float)pos.x),
            UNITS_TO_PIXELS((float)pos.y),
            (float)pos.width,
            (float)pos.height
        };
        static float prevX = 0.0f;
        static float prevY = 0.0f;
        //printf("sprite draw: %f %f delta: %f %f\n", dst.x, dst.y, dst.x - prevX, dst.y - prevY);
        prevX = dst.x;
        prevY = dst.y;
        DrawTextureTiled(sprite.spriteDef->spritesheet->texture, src, dst, { 0.0f, 0.0f }, 0.0f,
            (float)sprite.scale, color);
    } else {
        // Draw magenta rectangle
        Recti dest = pos;
        dest.x = UNITS_TO_PIXELS(dest.x);
        dest.y = UNITS_TO_PIXELS(dest.y);
        DrawRectangle(dest.x, dest.y, dest.width, dest.height, MAGENTA);
    }

#if DEMO_BODY_RECT
    // DEBUG: Draw bottom bottomCenter
    //Vector2 groundPos = sprite_world_center;
    //DrawCircle((int)groundPos.x, (int)groundPos.y, 4.0f, RED);
#endif
}

void sprite_draw_body(const Sprite &sprite, const Body3D &body, const Color &color)
{
    const Recti bodyRect = sprite_world_rect(sprite, body.position, sprite.scale);
    sprite_draw(sprite, bodyRect, color);
}
