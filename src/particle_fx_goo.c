#include "particles.h"
#include "particle_fx.h"
#include "dlb_rand.h"
#include <assert.h>

PARTICLE_FX_INIT(goo)
{
    // Spawn randomly during first 25% of duration
    particle->spawnAt = duration * dlb_rand32f_variance(0.25f);

    // Die randomly during last 15% of animation
    particle->dieAt = duration - (duration * dlb_rand32f_variance(0.15f));
    assert(particle->dieAt > particle->spawnAt);

#if 1
    float randX = dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
    float randY = dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
    float randZ = dlb_rand32f_range(0.0f, METERS_TO_PIXELS(2.0f));
    particle->body.velocity = (Vector3){ randX, randY, randZ };
    particle->body.friction = 0.5f;
#else
    const float direction = 1.0f;
    float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
    particle->velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
#endif
    //particle->position = (Vector2){ 0.0f, 0.0f };
    particle->sprite.scale = 1.0f;
    particle->color = (Color){ 154, 219, 63, 178 };  // Slime lime
}

PARTICLE_FX_UPDATE(goo)
{
    const float radius = 5.0f;
    // radius * 1.0 -> 0.2
    particle->sprite.scale = radius * (1.0f - alpha * 0.8f);
}
