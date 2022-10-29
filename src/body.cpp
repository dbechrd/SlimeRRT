#include "body.h"
#include "clock.h"
#include "helpers.h"
#include "maths.h"
#include "spritesheet.h"
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

inline Vector3 Body3D::GroundPosition3(void) const
{
    Vector3 groundPosition = { position.x, position.y, 0 };
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
    positionPrev = position;
    position = pos;

    position.x *= fabsf(position.x) >= POSITION_EPSILON;
    position.y *= fabsf(position.y) >= POSITION_EPSILON;
    position.z *= fabsf(position.z) >= POSITION_EPSILON;
    DLB_ASSERT(isfinite(position.x));
    DLB_ASSERT(isfinite(position.y));
    DLB_ASSERT(isfinite(position.z));

    lastMoved = g_clock.now;
}

inline void Body3D::Move(Vector2 offset)
{
    positionPrev = position;
    position.x += offset.x;
    position.y += offset.y;

    position.x *= fabsf(position.x) >= POSITION_EPSILON;
    position.y *= fabsf(position.y) >= POSITION_EPSILON;
    position.z *= fabsf(position.z) >= POSITION_EPSILON;
    DLB_ASSERT(isfinite(position.x));
    DLB_ASSERT(isfinite(position.y));
    DLB_ASSERT(isfinite(position.z));

    lastMoved = g_clock.now;
}

inline void Body3D::Move3D(Vector3 offset)
{
    positionPrev = position;
    position = v3_add(position, offset);

    position.x *= fabsf(position.x) >= POSITION_EPSILON;
    position.y *= fabsf(position.y) >= POSITION_EPSILON;
    position.z *= fabsf(position.z) >= POSITION_EPSILON;
    DLB_ASSERT(isfinite(position.x));
    DLB_ASSERT(isfinite(position.y));
    DLB_ASSERT(isfinite(position.z));

    lastMoved = g_clock.now;
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

    DLB_ASSERT(isfinite(position.x));
    DLB_ASSERT(isfinite(position.y));
    DLB_ASSERT(isfinite(position.z));
    DLB_ASSERT(isfinite(velocity.x));
    DLB_ASSERT(isfinite(velocity.y));
    DLB_ASSERT(isfinite(velocity.z));

    // Simulate physics if body not resting
    if (!Resting()) {
        const float gravity = -METERS_TO_PIXELS(10.0f) * gravityScale;
        const float accel = gravity;

        // Artificial damping, real drag would be in the direction opposite of motion and higher at higher speeds
        float damping_coef = 1.0f - CLAMP(drag, 0.0f, 1.0f);
        //velocity.x *= damping_coef;
        //velocity.y *= damping_coef;
        velocity.z *= damping_coef;

        velocity.z += accel * (float)dt; // * drag_coef;

        position.x += velocity.x * (float)dt;
        position.y += velocity.y * (float)dt;
        position.z += velocity.z * (float)dt;

        // Hitting ground
        if (position.z <= 0.0f) {
            // Clamp tiny velocities to zero
            const Vector3 zero = { 0, 0, 0 };
            const float accelDueToGravity = gravity * (float)dt;
            const float nonGravityVelocity = fabsf(velocity.z - accelDueToGravity);
            if (nonGravityVelocity < VELOCITY_EPSILON) {
                //E_DEBUG("Resting body since velocity due to gravity");
                velocity = zero;
                position.z = 0.0f;
            } else {
                // Bounce
                velocity.z *= -restitution;
                //position.z *= -restitution;
                position.z = 0.0f;
                bounced = true;
            }

            // Apply friction
            // TODO: Account for dt in friction?
            float friction_coef = 1.0f - CLAMP(friction, 0.0f, 1.0f);
            velocity.x *= friction_coef;
            velocity.y *= friction_coef;
            velocity.z *= friction_coef;
        }

        position.x *= fabsf(position.x) >= POSITION_EPSILON;
        position.y *= fabsf(position.y) >= POSITION_EPSILON;
        position.z *= fabsf(position.z) >= POSITION_EPSILON;
        DLB_ASSERT(isfinite(position.x));
        DLB_ASSERT(isfinite(position.y));
        DLB_ASSERT(isfinite(position.z));

        velocity.x *= fabsf(velocity.x) >= VELOCITY_EPSILON;
        velocity.y *= fabsf(velocity.y) >= VELOCITY_EPSILON;
        velocity.z *= fabsf(velocity.z) >= VELOCITY_EPSILON;
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
    //lastUpdated = g_clock.now;
}

bool Body3D::CL_Interpolate(double renderAt, Direction &direction)
{
    Vector3 startPos = WorldPosition();
    const size_t historyLen = positionHistory.Count();

    // If no history, nothing to interpolate yet
    if (historyLen == 0) {
        //printf("No snapshot history to interpolate from\n");
        return true;
    }

    // Find first snapshot after renderAt time
    size_t right = 0;
    while (right < historyLen && positionHistory.At(right).serverTime <= renderAt) {
        right++;
    }

    // renderAt is before any snapshots, show entity at oldest snapshot
    if (right == 0) {
        const Vector3Snapshot &oldest = positionHistory.At(0);
        DLB_ASSERT(renderAt < oldest.serverTime);
        //printf("renderAt %f before oldest snapshot %f\n", renderAt, oldest.serverTime);

        Teleport(oldest.v);
        direction = oldest.direction;
        // renderAt is after all snapshots, show entity at newest snapshot
    } else if (right == historyLen) {
        // TODO: Extrapolate beyond latest snapshot if/when this happens? Should be mostly avoidable..
        const Vector3Snapshot &newest = positionHistory.At(historyLen - 1);
        DLB_ASSERT(renderAt >= newest.serverTime);
        if (renderAt > newest.serverTime) {
            //printf("renderAt %f after newest snapshot %f\n", renderAt, newest.serverTime);
        }

        // TODO: Send explicit despawn event from server
        // If we haven't seen an entity in 2 snapshots, chances are it's gone
        if (renderAt > newest.serverTime + (1.0 / SNAPSHOT_SEND_RATE) * 2) {
            //printf("Despawning body due to inactivity\n");
            return false;
        }

        Teleport(newest.v);
        direction = newest.direction;
        // renderAt is between two snapshots
    } else {
        DLB_ASSERT(right > 0);
        DLB_ASSERT(right < historyLen);
        const Vector3Snapshot &a = positionHistory.At(right - 1);
        const Vector3Snapshot &b = positionHistory.At(right);

        if (renderAt < a.serverTime) {
            DLB_ASSERT(renderAt);
        }
        DLB_ASSERT(renderAt >= a.serverTime);
        DLB_ASSERT(renderAt < b.serverTime);

        // Linear interpolation: x = x0 + (x1 - x0) * alpha;
        double alpha = (renderAt - a.serverTime) / (b.serverTime - a.serverTime);
        const Vector3 interpPos = v3_add(a.v, v3_scale(v3_sub(b.v, a.v), (float)alpha));
        Teleport(interpPos);
        direction = b.direction;
    }

    Vector3 endPos = WorldPosition();

    if (!v3_equal(endPos, startPos, POSITION_EPSILON)) {
        lastMoved = g_clock.now;
    }
    jumped = (positionPrev.z == 0.0f && position.z > 0.0f);
    landed = (positionPrev.z > 0.0f && position.z == 0.0f);
    // TODO: Can't detect this client-side atm, would need to either sync velocity or a bounced flag, probably pointless
    //bounced = bounced && !v3_is_zero(velocity);

    const double timeSinceLastMove = g_clock.now - lastMoved;
    bool prevIdle = idle;
    idle = timeSinceLastMove > IDLE_THRESHOLD_SECONDS;
    idleChanged = idle != prevIdle;
    //lastUpdated = g_clock.now;

    return true;
}