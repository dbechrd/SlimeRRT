#include "body.h"
#include "helpers.h"
#include "maths.h"
#include "spritesheet.h"
#include "GLFW/glfw3.h"
#include <cassert>
#include <cmath>

#define VELOCITY_EPSILON 0.001f
#define IDLE_THRESHOLD_SECONDS 6.0

Vector3i Body3D::BottomCenter() const
{
    Vector3i result = GroundPosition();
    result.y -= position.z;
    return result;
}

Vector3i Body3D::GroundPosition() const
{
    Vector3i groundPosition = { position.x, position.y, 0 };
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
        const int gravity = METERS_TO_PIXELS(10);
        velocity.z -= (int)(gravity * dt); // * drag_coef;

        position.x += (int)(velocity.x * dt);
        position.y += (int)(velocity.y * dt);
        position.z += (int)(velocity.z * dt);

        bounced = position.z <= 0.0f;

        float friction_coef = 1.0f;
        if (position.z <= 0.0f) {
            // Bounce
            velocity.z = (int)(velocity.z * -restitution);
            position.z = (int)(position.z * -restitution);

            // Apply friction
            // TODO: Account for dt in friction?
            friction_coef = 1.0f - CLAMP(friction, 0.0f, 1.0f);
            velocity.x = (int)(velocity.x * friction_coef);
            velocity.y = (int)(velocity.y * friction_coef);
        }
    }

    // NOTE: Position can be updated manually outside of physics sim (e.g. player Move() controller)
    if (!v3_equal(position, prevPosition)) {
        lastMoved = glfwGetTime();
    }
    landed = (prevPosition.z > 0 && position.z == 0);
    bounced = bounced && !v3_is_zero(velocity);

    const double timeSinceLastMove = glfwGetTime() - lastMoved;
    bool prevIdle = idle;
    idle = timeSinceLastMove > IDLE_THRESHOLD_SECONDS;
    idleChanged = idle != prevIdle;
    lastUpdated = glfwGetTime();
    prevPosition = position;
}