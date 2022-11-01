#include "../particles.h"
#include "dlb_rand.h"

namespace FX {
    void goo_init(Particle &particle)
    {
        DLB_ASSERT(particle.effect);
        const ParticleEffect &effect = *particle.effect;

        // Spawn randomly during first 25% of duration
        particle.spawnAt = effect.startedAt + effect.duration * (double)dlb_rand32f_variance(0.25f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.startedAt + effect.duration - (effect.duration * (double)dlb_rand32f_variance(0.15f));
        DLB_ASSERT(particle.dieAt > particle.spawnAt);

    #if 1
        float randX = dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
        float randY = dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
        float randZ = dlb_rand32f_range(0.0f, METERS_TO_PIXELS(2.0f));
        particle.body.velocity = { randX, randY, randZ };
        particle.body.friction = 0.5f;
    #else
        const float direction = 1.0f;
        float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
        particle.velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
    #endif
        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = { 154, 219, 63, 178 };  // Slime lime
        particle.sprite.scale = 1.0f;
    }

    void goo_update(Particle &particle, float alpha)
    {
        const float radius = 10.0f;
        // radius * 1.0 -> 0.2
        particle.sprite.scale = radius * (1.0f - alpha * 0.8f);
    }
}