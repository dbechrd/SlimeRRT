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
    const SpriteAnim &anim = sheet->animations[animationIdx];
    return anim;
}

const SpriteFrame &sprite_frame(const Sprite &sprite)
{
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

Rectangle sprite_frame_rect(const Sprite &sprite)
{
    const SpriteFrame &frame = sprite_frame(sprite);
    Rectangle rect = {};
    rect.x = (float)frame.x;
    rect.y = (float)frame.y;
    rect.width = (float)frame.width;
    rect.height = (float)frame.height;
    return rect;
}

Rectangle sprite_world_rect(const Sprite &sprite, Vector3 position, float scale)
{
    Rectangle frameRect = sprite_frame_rect(sprite);
    Rectangle rect = {};
    rect.x = position.x - frameRect.width / 2.0f * scale;
    rect.y = position.y - frameRect.height * scale - position.z;
    rect.width = frameRect.width * scale;
    rect.height = frameRect.height * scale;
    return rect;
}

Vector3 sprite_world_top_center(const Sprite &sprite, Vector3 position, float scale)
{
    Rectangle frameRect = sprite_frame_rect(sprite);
    Vector3 center = {};
    center.x = position.x;
    center.y = position.y;
    center.z = position.z + frameRect.height * scale;
    return center;
}

Vector3 sprite_world_center(const Sprite &sprite, Vector3 position, float scale)
{
    Rectangle frameRect = sprite_frame_rect(sprite);
    Vector3 center = {};
    center.x = position.x;
    center.y = position.y;
    center.z = position.z + frameRect.height / 2.0f * scale;
    return center;
}

void sprite_update(Sprite &sprite, double now, double dt)
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
        if (now - sprite.lastAnimFrameStarted > animDelay) {
            sprite.animFrameIdx++;
            sprite.animFrameIdx %= anim.frameCount;
            sprite.lastAnimFrameStarted = now;
        }
    }
}

bool sprite_cull_body(const Sprite &sprite, const Body3D &body, Rectangle cullRect)
{
    const Rectangle bodyRect = sprite_world_rect(sprite, body.position, sprite.scale);
    bool cull = !CheckCollisionRecs(bodyRect, cullRect);
    return cull;
}

static void sprite_draw(const Sprite &sprite, Rectangle dest, Color color)
{
#if DEMO_BODY_RECT
    // DEBUG: Draw collision rectangle
    DrawRectangleRec(dest, Fade(RED, 0.2f));
#endif

    if (sprite.spriteDef) {
#if 0
        // Funny bug where texture stays still relative to screen, could be fun to abuse later
        const Rectangle rect = drawthing_rect(drawThing);
#else
        const Rectangle rect = sprite_frame_rect(sprite);
#endif
        // Draw textured sprite
        DrawTextureTiled(sprite.spriteDef->spritesheet->texture, rect, dest, { 0.0f, 0.0f }, 0.0f,
            sprite.scale, color);
    } else {
        // Draw magenta rectangle
        DrawRectangleRec(dest, MAGENTA);
    }

#if DEMO_BODY_RECT
    // DEBUG: Draw bottom bottomCenter
    //Vector2 groundPos = sprite_world_center;
    //DrawCircle((int)groundPos.x, (int)groundPos.y, 4.0f, RED);
#endif
}

void sprite_draw_body(const Sprite &sprite, const Body3D &body, Color color)
{
    const Rectangle bodyRect = sprite_world_rect(sprite, body.position, sprite.scale);
    sprite_draw(sprite, bodyRect, color);
}
