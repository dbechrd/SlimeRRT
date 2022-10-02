#include "body.h"
#include "clock.h"
#include "helpers.h"
#include "maths.h"
#include "spritesheet.h"
#include <cassert>
#include <cmath>

#define IDLE_THRESHOLD_SECONDS 60.0

Body3D::Body3D(void)
{
    gravityScale = 1.0f;
}

#if GF_SKIP_BODY_FLOORF
    #define floorf(x) (x)
#endif

inline Vector3 Body3D::WorldPosition(void) const
{
    Vector3 worldPosition = position;
    worldPosition.x = floorf(worldPosition.x);
    worldPosition.y = floorf(worldPosition.y);
    worldPosition.z = floorf(worldPosition.z);
    return worldPosition;
}

inline Vector3 Body3D::WorldPositionServer(void) const
{
    Vector3 worldPositionServer{};
    if (positionHistory.Count()) {
        Vector3Snapshot snapshotPos = positionHistory.Last();
        worldPositionServer.x = floorf(snapshotPos.v.x);
        worldPositionServer.y = floorf(snapshotPos.v.y);
        worldPositionServer.z = floorf(snapshotPos.v.z);
    }
    return worldPositionServer;
}

inline Vector2 Body3D::GroundPosition(void) const
{
    Vector2 groundPosition = { position.x, position.y };
    groundPosition.x = floorf(groundPosition.x);
    groundPosition.y = floorf(groundPosition.y);
    return groundPosition;
}

inline Vector2 Body3D::GroundPositionServer(void) const
{
    Vector2 groundPositionServer{};
    if (positionHistory.Count()) {
        Vector3Snapshot snapshotPos = positionHistory.Last();
        groundPositionServer.x = floorf(snapshotPos.v.x);
        groundPositionServer.y = floorf(snapshotPos.v.y);
    }
    return groundPositionServer;
}

inline Vector2 Body3D::PrevGroundPosition(void) const
{
    Vector2 prevGroundPosition = { positionPrev.x, positionPrev.y };
    prevGroundPosition.x = floorf(prevGroundPosition.x);
    prevGroundPosition.y = floorf(prevGroundPosition.y);
    return prevGroundPosition;
}

inline Vector2 Body3D::VisualPosition(void) const
{
    Vector2 visualPosition = GroundPosition();
    visualPosition.y -= position.z;
    visualPosition.x = floorf(visualPosition.x);
    visualPosition.y = floorf(visualPosition.y);
    return visualPosition;
}

#undef floorf

inline void Body3D::Teleport(Vector3 pos)
{
    destPosition.x = pos.x;
    destPosition.y = pos.y;
    destPosition.z = pos.z;

    positionPrev = position;
    position = destPosition;
    lastMoved = g_clock.now;
}

inline void Body3D::Move(Vector2 offset)
{
    destPosition.x += offset.x;
    destPosition.y += offset.y;
}

inline void Body3D::Move3D(Vector3 offset)
{
    destPosition.x += offset.x;
    destPosition.y += offset.y;
    destPosition.z += offset.z;
}

inline bool Body3D::OnGround(void) const
{
    return position.z == 0.0f;
}

inline bool Body3D::Resting(void) const
{
    return v3_is_zero(velocity) && OnGround();
}

inline bool Body3D::Idle(void) const
{
    return idle;
}

inline bool Body3D::Jumped(void) const
{
    return jumped;
}

inline bool Body3D::Landed(void) const
{
    return landed;
}

inline bool Body3D::Bounced(void) const
{
    return bounced;
}

inline double Body3D::TimeSinceLastMove(void) const
{
    return g_clock.now - lastMoved;
}

inline void Body3D::ApplyForce(Vector3 force)
{
    velocity.x += force.x;
    velocity.y += force.y;
    velocity.z += force.z;
}

void Body3D::Update(double dt)
{
    positionPrev = position;
    position = destPosition;

    DLB_ASSERT(isfinite(position.x));
    DLB_ASSERT(isfinite(position.y));
    DLB_ASSERT(isfinite(position.z));
    DLB_ASSERT(isfinite(velocity.x));
    DLB_ASSERT(isfinite(velocity.y));
    DLB_ASSERT(isfinite(velocity.z));

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
                //E_DEBUG("Resting body since velocity due to gravity");
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

        DLB_ASSERT(isfinite(position.x));
        DLB_ASSERT(isfinite(position.y));
        DLB_ASSERT(isfinite(position.z));
        DLB_ASSERT(isfinite(velocity.x));
        DLB_ASSERT(isfinite(velocity.y));
        DLB_ASSERT(isfinite(velocity.z));
    }

    if (!v3_equal(position, positionPrev, POSITION_EPSILON)) {
        lastMoved = g_clock.now;
    }
    jumped = (positionPrev.z == 0.0f && position.z > 0.0f);
    landed = (positionPrev.z > 0.0f && position.z == 0.0f);
    bounced = bounced && !v3_is_zero(velocity);

    const double timeSinceLastMove = g_clock.now - lastMoved;
    bool prevIdle = idle;
    idle = timeSinceLastMove > IDLE_THRESHOLD_SECONDS;
    idleChanged = idle != prevIdle;
    lastUpdated = g_clock.now;
    destPosition = position;
}