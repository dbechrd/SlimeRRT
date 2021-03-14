#include "body.h"
#include "helpers.h"
#include "math.h"
#include "spritesheet.h"
#include <assert.h>
#include <math.h>

#define VELOCITY_EPSILON 0.001f

SpriteAnim *body_anim(const Body3D *body)
{
    assert(body);
    assert(body->sprite);
    assert(body->sprite->spritesheet);
    assert(body->sprite->animations);

    const Spritesheet *sheet = body->sprite->spritesheet;
    const int animationIdx = body->sprite->animations[body->facing];
    SpriteAnim *anim = &sheet->animations[animationIdx];
    return anim;
}

SpriteFrame *body_frame(const Body3D *body)
{
    assert(body);
    assert(body->sprite);
    assert(body->sprite->spritesheet);

    SpriteAnim *animation = body_anim(body);
    assert(body->animFrameIdx >= 0);
    assert(body->animFrameIdx < animation->frameCount);

    const int frameIdx = animation->frames[body->animFrameIdx];
    Spritesheet *sheet = body->sprite->spritesheet;
    assert(frameIdx >= 0);
    assert(frameIdx < sheet->frameCount);

    SpriteFrame *frame = &sheet->frames[frameIdx];
    return frame;
}

Rectangle body_frame_rect(const Body3D *body)
{
    assert(body);

    const SpriteFrame *frame = body_frame(body);
    Rectangle rect = { 0 };
    rect.x = (float)frame->x;
    rect.y = (float)frame->y;
    rect.width = (float)frame->width;
    rect.height = (float)frame->height;
    return rect;
}

Rectangle body_rect(const Body3D *body)
{
    assert(body);

    const Rectangle frameRect = body_frame_rect(body);
    Rectangle rect = { 0 };
    rect.x = body->position.x - frameRect.width / 2.0f * body->scale;
    rect.y = body->position.y - frameRect.height * body->scale - body->position.z;
    rect.width = frameRect.width * body->scale;
    rect.height = frameRect.height * body->scale;
    return rect;
}

Vector3 body_center(const Body3D *body)
{
    assert(body);

    const SpriteFrame *spriteFrame = body_frame(body);
    Vector3 center = { 0 };
    center.x = body->position.x;
    center.y = body->position.y;
    center.z = body->position.z + spriteFrame->height / 2.0f * body->scale;
    return center;
}

Vector2 body_ground_position(const Body3D *body)
{
    assert(body);

    Vector2 groundPosition = { 0 };
    groundPosition.x = body->position.x;
    groundPosition.y = body->position.y;
    return groundPosition;
}

void body_update(Body3D *body, double now, double dt)
{
    assert(body);

    // animation
    // TODO: SpriteAnimation struct that stores array of animFrameIdx/delay pairs
    // HACK: Update sprite animation manually
    if (body->sprite) {
        const SpriteAnim *anim = body_anim(body);
        if (anim && anim->frameCount > 1) {
            const double animFps = 24.0;
            const double animDelay = 1.0 / animFps;
            if (now - body->lastAnimFrameStarted > animDelay) {
                body->animFrameIdx++;
                body->animFrameIdx %= anim->frameCount;
                body->lastAnimFrameStarted = now;
            }
        }
    }

    body->lastUpdated = now;
    const Vector3 prevPosition = body->position;

    // TODO: Account for dt in drag (How? exp()? I forgot..)
    //const float drag_coef = 1.0f - CLAMP(body->drag, 0.0f, 1.0f);
    //body->velocity.x += body->acceleration.x * dt * drag_coef;
    //body->velocity.y += body->acceleration.y * dt * drag_coef;
    //body->velocity.z += body->acceleration.z * dt * drag_coef;

    // Resting on ground
    if (v3_is_zero(body->velocity) && body->position.z == 0.0f) {
        return;
    }

    const float gravity = METERS(10.0f);
    body->velocity.z -= gravity * (float)dt; // * drag_coef;

    body->position.x += body->velocity.x * (float)dt;
    body->position.y += body->velocity.y * (float)dt;
    body->position.z += body->velocity.z * (float)dt;

    float friction_coef = 1.0f;
    if (body->position.z <= 0.0f) {
        // Bounce
        body->velocity.z *= -body->restitution;
        body->position.z *= -body->restitution;

        // Apply friction
        // TODO: Account for dt in friction?
        friction_coef = 1.0f - CLAMP(body->friction, 0.0f, 1.0f);
        body->velocity.x *= friction_coef;
        body->velocity.y *= friction_coef;
    }

    // TODO: Epsilon could be defined per body? Idk if that's useful enough to be worth it
    // Clamp tiny velocities to zero
    if (fabsf(body->velocity.x) < VELOCITY_EPSILON) body->velocity.x = 0.0f;
    if (fabsf(body->velocity.y) < VELOCITY_EPSILON) body->velocity.y = 0.0f;
    if (fabsf(body->velocity.z) < VELOCITY_EPSILON) body->velocity.z = 0.0f;

    if (!v3_equal(body->position, prevPosition)) {
        body->lastMoved = now;
    }
}

void body_draw(const Body3D *body)
{
#if 0
    // Funny bug where texture stays still relative to screen, could be fun to abuse later
    const Rectangle rect = body_rect(body);
#endif
    const Rectangle rect = body_frame_rect(body);
    //DrawTextureRec(*body->sprite->spritesheet->texture, rect, body->transform.position, WHITE);

    Rectangle dest = body_rect(body);

#if DEMO_BODY_RECT
    // DEBUG: Draw collision rectangle
    DrawRectangleRec(dest, Fade(RED, 0.2f));
#endif

    // Draw textured sprite
    DrawTextureTiled(body->sprite->spritesheet->texture, rect, dest, (Vector2){ 0.0f, 0.0f }, 0.0f, body->scale,
        body->color);

#if DEMO_BODY_RECT
    // DEBUG: Draw bottom bottomCenter
    Vector2 groundPos = body_ground_position(body);
    DrawCircle((int)groundPos.x, (int)groundPos.y, 4.0f, RED);
#endif
}