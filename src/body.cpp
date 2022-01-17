#include "body.h"
#include "helpers.h"
#include "maths.h"
#include "spritesheet.h"
#include "GLFW/glfw3.h"
#include <cassert>
#include <cmath>

#define VELOCITY_EPSILON 0.001f
#define IDLE_THRESHOLD_SECONDS 6.0

inline Vector3 Body3D::WorldPosition() const
{
    return position;
}

inline Vector2 Body3D::GroundPosition() const
{
    Vector2 groundPosition = { position.x, position.y };
    return groundPosition;
}

inline Vector2 Body3D::PrevGroundPosition() const
{
    Vector2 prevGroundPosition = { prevPosition.x, prevPosition.y };
    return prevGroundPosition;
}

inline Vector2 Body3D::VisualPosition() const
{
    Vector2 result = GroundPosition();
    result.y -= position.z;
    return result;
}

inline void Body3D::Teleport(const Vector3 &pos)
{
    prevPosition = position;
    position.x = pos.x;
    position.y = pos.y;
    position.z = pos.z;
    lastMoved = glfwGetTime();
}

inline void Body3D::Move(const Vector2 &offset)
{
    prevPosition = position;
    position.x += offset.x;
    position.y += offset.y;
    lastMoved = glfwGetTime();
}

inline bool Body3D::Bounced() const
{
    return bounced;
}

inline bool Body3D::OnGround() const
{
    return position.z == 0.0f;
}

inline bool Body3D::JustLanded(void) const
{
    return landed;
}

inline bool Body3D::Resting() const
{
    return v3_is_zero(velocity) && OnGround();
}

inline bool Body3D::Idle() const
{
    return idle;
}

inline double Body3D::TimeSinceLastMove() const
{
    return glfwGetTime() - lastMoved;
}

inline void Body3D::ApplyForce(const Vector3 &force)
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
    lastUpdated = glfwGetTime();
}