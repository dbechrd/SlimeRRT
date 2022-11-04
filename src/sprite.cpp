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
    assert(animationIdx >= 0 && animationIdx < (int)sheet->animations.size());
    const SpriteAnim &anim = sheet->animations[animationIdx];
    return anim;
}

const SpriteFrame &sprite_frame(const Sprite &sprite)
{
    if (!sprite.spriteDef) {
        // TODO: Wtf is this? Who is using this?
        thread_local static SpriteFrame frame{ 0 };
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
    assert(frameIdx < (int)sheet->frames.size());

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

// pos: bottom center position
// returns sprite rect in world space
Rectangle sprite_world_rect(const Sprite &sprite, const Vector3 &pos)
{
    Rectangle frameRect = sprite_frame_rect(sprite);
    Rectangle rect{};
    rect.x = pos.x - frameRect.width / 2.0f * sprite.scale;
    rect.y = pos.y - pos.z - frameRect.height * sprite.scale;
    rect.width = frameRect.width * sprite.scale;
    rect.height = frameRect.height * sprite.scale;
    return rect;
}

// returns offset of top center from bottom center
Vector3 sprite_top_center(const Sprite &sprite)
{
    Rectangle frameRect = sprite_frame_rect(sprite);
    Vector3 topCenter = { 0, 0, frameRect.height * sprite.scale };
    return topCenter;
}

// returns offset of center from bottom center
Vector3 sprite_center(const Sprite &sprite)
{
    Rectangle frameRect = sprite_frame_rect(sprite);
    Vector3 center = { 0, 0, frameRect.height / 2.0f * sprite.scale };
    return center;
}

void sprite_set_direction(Sprite &sprite, Vector2 offset)
{
#if 0
    // 4-directional mode
    Direction dir = sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            dir = fabs(offset.x) > fabs(offset.y) ? Direction::East : Direction::South;
        } else if (offset.y < 0.0f) {
            dir = fabs(offset.x) > fabs(offset.y) ? Direction::East : Direction::North;
        } else {
            dir = Direction::East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            dir = fabs(offset.x) > fabs(offset.y) ? Direction::West : Direction::South;
        } else if (offset.y < 0.0f) {
            dir = fabs(offset.x) > fabs(offset.y) ? Direction::West : Direction::North;
        } else {
            dir = Direction::West;
        }
    } else {
        if (offset.y > 0.0f) {
            dir = Direction::South;
        } else if (offset.y < 0.0f) {
            dir = Direction::North;
        }
    }
    if (dir != sprite.direction) {
        sprite.direction = dir;
        sprite.animFrameIdx = 0;
    }
#else
    // 8-directional mode
    Direction dir = sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            dir = Direction::SouthEast;
        } else if (offset.y < 0.0f) {
            dir = Direction::NorthEast;
        } else {
            dir = Direction::East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            dir = Direction::SouthWest;
        } else if (offset.y < 0.0f) {
            dir = Direction::NorthWest;
        } else {
            dir = Direction::West;
        }
    } else {
        if (offset.y > 0.0f) {
            dir = Direction::South;
        } else if (offset.y < 0.0f) {
            dir = Direction::North;
        }
    }
    if (dir != sprite.direction) {
        sprite.direction = dir;
        sprite.animFrameIdx = 0;
    }
#endif
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
        if (g_clock.now - sprite.lastAnimFrameStarted > animDelay) {
            sprite.animFrameIdx++;
            sprite.animFrameIdx %= anim.frameCount;
            sprite.lastAnimFrameStarted = g_clock.now;
        }
    }
}

bool sprite_cull_body(const Sprite &sprite, const Body3D &body, Rectangle cullRect)
{
    const Vector3 worldPos = body.WorldPosition();
    const Rectangle bodyRect = sprite_world_rect(sprite, worldPos);
    bool cull = !CheckCollisionRecs(bodyRect, cullRect);
    return cull;
}

static void sprite_draw(const Sprite &sprite, Rectangle screenRect, Color color)
{
#if CL_DEMO_BODY_RECT
    // DEBUG: Draw collision rectangle
    DrawRectangleRec(dest, Fade(RED, 0.2f));
#endif

    if (sprite.spriteDef) {
        // Draw textured sprite
        const Rectangle sheetRect = sprite_frame_rect(sprite);
        DrawTextureTiled(sprite.spriteDef->spritesheet->texture, sheetRect, screenRect, { 0.0f, 0.0f }, 0.0f,
            sprite.scale, color);
    } else {
        // Draw colored rectangle
        DrawRectangleRec(screenRect, color);
    }

#if CL_DEMO_BODY_RECT
    // DEBUG: Draw bottom bottomCenter
    //Vector2 groundPos = sprite_world_center;
    //DrawCircle((int)groundPos.x, (int)groundPos.y, 4.0f, RED);
#endif
}

void sprite_draw_body(const Sprite &sprite, const Body3D &body, const Color &color)
{
    if (g_cl_show_snapshot_shadow) {
        #if 1
            Vector3 serverPos = body.WorldPositionServer();
            serverPos.x = floorf(serverPos.x);
            serverPos.y = floorf(serverPos.y);
            serverPos.z = floorf(serverPos.z);
            const Rectangle serverRect = sprite_world_rect(sprite, serverPos);
            sprite_draw(sprite, serverRect, GRAY);
        #else
            int posCount = (int)body.positionHistory.Count();
            for (int i = 0; i < body.positionHistory.Count(); i++) {
                Vector3 serverPos{};
                if (body.positionHistory.Count()) {
                    const Vector3Snapshot &snapshotPos = body.positionHistory.At(i);
                    serverPos.x = floorf(snapshotPos.v.x);
                    serverPos.y = floorf(snapshotPos.v.y);
                    serverPos.z = floorf(snapshotPos.v.z);
                }
                const Rectangle serverRect = sprite_world_rect(sprite, serverPos);

                uint8_t gray = (uint8_t)((float)(i + 1) / posCount * 255);
                sprite_draw(sprite, serverRect, { gray, gray, gray, 50 });
            }
        #endif
    }

    Vector3 worldPos = body.WorldPosition();
    worldPos.x = floorf(worldPos.x);
    worldPos.y = floorf(worldPos.y);
    worldPos.z = floorf(worldPos.z);
    //DrawCircle(worldPos.x, worldPos.y - worldPos.z, 3.0f, RED);
    const Rectangle bodyRect = sprite_world_rect(sprite, worldPos);
    sprite_draw(sprite, bodyRect, color);
}
