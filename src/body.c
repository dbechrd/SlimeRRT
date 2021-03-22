#include "body.h"
#include "helpers.h"
#include "maths.h"
#include "spritesheet.h"
#include <assert.h>
#include <math.h>

#define VELOCITY_EPSILON 0.001f
#define IDLE_THRESHOLD_SECONDS 6.0

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

    body->lastUpdated = now;
    const Vector3 prevPosition = body->position;

    // TODO: Account for dt in drag (How? exp()? I forgot..)
    //const float drag_coef = 1.0f - CLAMP(drawThing->drag, 0.0f, 1.0f);
    //drawThing->velocity.x += drawThing->acceleration.x * dt * drag_coef;
    //drawThing->velocity.y += drawThing->acceleration.y * dt * drag_coef;
    //drawThing->velocity.z += drawThing->acceleration.z * dt * drag_coef;

    // Resting on ground
    if (v3_is_zero(body->velocity) && body->position.z == 0.0f) {
        return;
    }

    const float gravity = METERS_TO_PIXELS(10.0f);
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

    // TODO: Epsilon could be defined per drawThing? Idk if that's useful enough to be worth it
    // Clamp tiny velocities to zero
    if (fabsf(body->velocity.x) < VELOCITY_EPSILON) body->velocity.x = 0.0f;
    if (fabsf(body->velocity.y) < VELOCITY_EPSILON) body->velocity.y = 0.0f;
    if (fabsf(body->velocity.z) < VELOCITY_EPSILON) body->velocity.z = 0.0f;

    if (!v3_equal(body->position, prevPosition)) {
        body->lastMoved = now;
    }
    body->landed = (prevPosition.z > 0.0f && body->position.z == 0.0f);

    const double timeSinceLastMove = now - body->lastMoved;
    body->idle = timeSinceLastMove > IDLE_THRESHOLD_SECONDS;
}