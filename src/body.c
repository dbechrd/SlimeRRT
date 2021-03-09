#include "body.h"
#include "helpers.h"
#include <assert.h>
#include <math.h>

#define VELOCITY_EPSILON 0.001f

void body3d_update(Body3D *body, float dt)
{
    assert(body);

    // TODO: Account for dt (How? exp()? I forgot..)
    const float drag_coef = 1.0f - CLAMP(body->drag, 0.0f, 1.0f);
    body->velocity.x += body->acceleration.x * dt * drag_coef;
    body->velocity.y += body->acceleration.y * dt * drag_coef;
    body->velocity.z += body->acceleration.z * dt * drag_coef;

    body->position.x += body->velocity.x * dt;
    body->position.y += body->velocity.y * dt;
    body->position.z += body->velocity.z * dt;

    float friction_coef = 1.0f;
    if (body->position.z <= 0.0f) {
        // Bounce
        body->velocity.z *= -body->restitution;
        body->position.z *= -body->restitution;

        // Apply friction
        // TODO: Account for dt?
        friction_coef = 1.0f - CLAMP(body->friction, 0.0f, 1.0f);
        body->velocity.x *= friction_coef;
        body->velocity.y *= friction_coef;
    }

    // TODO: Epsilon could be defined per body? Idk if that's useful enough to be worth it
    // Clamp tiny velocities to zero
    if (fabsf(body->velocity.x) < VELOCITY_EPSILON) body->velocity.x = 0.0f;
    if (fabsf(body->velocity.y) < VELOCITY_EPSILON) body->velocity.y = 0.0f;
    if (fabsf(body->velocity.z) < VELOCITY_EPSILON) body->velocity.z = 0.0f;
}