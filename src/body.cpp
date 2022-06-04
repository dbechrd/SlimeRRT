#include "body.h"
#include "helpers.h"
#include "maths.h"
#include "spritesheet.h"
#include "GLFW/glfw3.h"
#include <cassert>
#include <cmath>

#define IDLE_THRESHOLD_SECONDS 60.0

Body3D::Body3D(void)
{
    gravityScale = 1.0f;
}

inline Vector3 Body3D::WorldPosition(void) const
{
    return position;
}

inline Vector2 Body3D::GroundPosition(void) const
{
    Vector2 groundPosition = { position.x, position.y };
    return groundPosition;
}

inline Vector2 Body3D::PrevGroundPosition(void) const
{
    Vector2 prevGroundPosition = { prevPosition.x, prevPosition.y };
    return prevGroundPosition;
}

inline Vector2 Body3D::VisualPosition(void) const
{
    Vector2 result = GroundPosition();
    result.y -= position.z;
    return result;
}

inline void Body3D::Teleport(Vector3 pos)
{
    prevPosition = position;
    position.x = pos.x;
    position.y = pos.y;
    position.z = pos.z;
    lastMoved = glfwGetTime();
}

inline void Body3D::Move(Vector2 offset)
{
    prevPosition = position;
    position.x += offset.x;
    position.y += offset.y;
    lastMoved = glfwGetTime();
}

inline void Body3D::Move3D(Vector3 offset)
{
    prevPosition = position;
    position.x += offset.x;
    position.y += offset.y;
    position.z += offset.z;
    lastMoved = glfwGetTime();
}

inline bool Body3D::Bounced(void) const
{
    return bounced;
}

inline bool Body3D::OnGround(void) const
{
    return position.z == 0.0f;
}

inline bool Body3D::JustLanded(void) const
{
    return landed;
}

inline bool Body3D::Resting(void) const
{
    return v3_is_zero(velocity) && OnGround();
}

inline bool Body3D::Idle(void) const
{
    return idle;
}

inline double Body3D::TimeSinceLastMove(void) const
{
    return glfwGetTime() - lastMoved;
}

inline void Body3D::ApplyForce(Vector3 force)
{
    velocity.x += force.x;
    velocity.y += force.y;
    velocity.z += force.z;
}

void Body3D::Update(double dt)
{
    prevPosition = position;

    // Simulate physics if body not resting
    if (!Resting()) {
        const float gravity = METERS_TO_PIXELS(10.0f);
        velocity.z -= gravity * gravityScale * (float)dt; // * drag_coef;

        position.x += velocity.x * (float)dt;
        position.y += velocity.y * (float)dt;
        position.z += velocity.z * (float)dt;

        float friction_coef = 1.0f;
        if (position.z <= 0.0f) {
            // Clamp tiny velocities to zero
            const Vector3 zero = { 0, 0, 0 };
            if ((fabsf(velocity.z) - gravity * (float)dt) < VELOCITY_EPSILON) {
                //TraceLog(LOG_DEBUG, "Resting body since velocity due to gravity");
                velocity = zero;
                position.z = 0.0f;
            } else {
                // Bounce
                velocity.z *= -restitution;
                position.z *= -restitution;
                bounced = true;

                // Apply friction
                // TODO: Account for dt in friction?
                friction_coef = 1.0f - CLAMP(friction, 0.0f, 1.0f);
                velocity.x *= friction_coef;
                velocity.y *= friction_coef;
            }
        }
    }

    // NOTE: Position can be updated manually outside of physics sim (e.g. player Move() controller)
    if (!v3_equal(position, prevPosition, POSITION_EPSILON)) {
        lastMoved = glfwGetTime();
    }
    landed = (prevPosition.z > 0.0f && position.z == 0.0f);
    bounced = bounced && !v3_is_zero(velocity);

    const double timeSinceLastMove = glfwGetTime() - lastMoved;
    bool prevIdle = idle;
    idle = timeSinceLastMove > IDLE_THRESHOLD_SECONDS;
    idleChanged = idle != prevIdle;
    lastUpdated = glfwGetTime();
}