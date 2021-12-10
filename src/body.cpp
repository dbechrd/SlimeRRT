#include "body.h"
#include "helpers.h"
#include "maths.h"
#include "spritesheet.h"
#include "GLFW/glfw3.h"
#include <cassert>
#include <cmath>

#define VELOCITY_EPSILON 0.001f
#define IDLE_THRESHOLD_SECONDS 6.0

Vector2 Body3D::BottomCenter() const
{
    Vector2 result = GroundPosition();
    result.y -= position.z;
    return result;
}

Vector2 Body3D::GroundPosition() const
{
    Vector2 groundPosition = { position.x, position.y };
    return groundPosition;
}

bool Body3D::OnGround() const
{
    return position.z == 0.0f;
}

bool Body3D::Resting() const
{
    return v3_is_zero(velocity) && OnGround();
}

void Body3D::Update(double dt)
{
    // TODO: Account for dt in drag (How? exp()? I forgot..)
    //const float drag_coef = 1.0f - CLAMP(drawThing->drag, 0.0f, 1.0f);
    //drawThing->velocity.x += drawThing->acceleration.x * dt * drag_coef;
    //drawThing->velocity.y += drawThing->acceleration.y * dt * drag_coef;
    //drawThing->velocity.z += drawThing->acceleration.z * dt * drag_coef;

    // Simulate physics if body not resting
    if (!Resting()) {
        const float gravity = METERS_TO_PIXELS(10.0f);
        velocity.z -= gravity * (float)dt; // * drag_coef;

        position.x += velocity.x * (float)dt;
        position.y += velocity.y * (float)dt;
        position.z += velocity.z * (float)dt;

        bounced = position.z <= 0.0f;

        float friction_coef = 1.0f;
        if (position.z <= 0.0f) {
            // Bounce
            velocity.z *= -restitution;
            position.z *= -restitution;

            // Apply friction
            // TODO: Account for dt in friction?
            friction_coef = 1.0f - CLAMP(friction, 0.0f, 1.0f);
            velocity.x *= friction_coef;
            velocity.y *= friction_coef;
        }

        // TODO: Epsilon could be defined per drawThing? Idk if that's useful enough to be worth it
        // Clamp tiny velocities to zero
        if (fabsf(velocity.x) < VELOCITY_EPSILON) velocity.x = 0.0f;
        if (fabsf(velocity.y) < VELOCITY_EPSILON) velocity.y = 0.0f;
        if (fabsf(velocity.z) < VELOCITY_EPSILON) velocity.z = 0.0f;
    }

    // NOTE: Position can be updated manually outside of physics sim (e.g. player Move() controller)
    if (!v3_equal(position, prevPosition)) {
        lastMoved = glfwGetTime();
    }
    landed = (prevPosition.z > 0.0f && position.z == 0.0f);
    bounced = bounced && !v3_is_zero(velocity);

    const double timeSinceLastMove = glfwGetTime() - lastMoved;
    bool prevIdle = idle;
    idle = timeSinceLastMove > IDLE_THRESHOLD_SECONDS;
    idleChanged = idle != prevIdle;

    prevPosition = position;
    lastUpdated = glfwGetTime();
}