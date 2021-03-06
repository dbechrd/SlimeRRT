#include "transform.h"
#include <assert.h>

void transform_update(Transform2D *transform, float dt)
{
    assert(transform);
    transform->velocity.x += transform->acceleration.x * dt;
    transform->velocity.y += transform->acceleration.y * dt;
    transform->position.x += transform->velocity.x * dt;
    transform->position.y += transform->velocity.y * dt;
}