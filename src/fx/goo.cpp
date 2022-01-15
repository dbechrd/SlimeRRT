#include "particles.h"
#include "dlb_rand.h"
#include <cassert>

namespace FX {
    void goo_init(Particle &particle, ParticleEffect &effect)
    {
        // Spawn randomly during first 25% of duration
        particle.spawnAt = effect.duration * dlb_rand32f_variance(0.25f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.duration - (effect.duration * dlb_rand32f_variance(0.15f));
        assert(particle.dieAt > particle.spawnAt);

    #if 1
        Vector3i randVel {};
        randVel.x = (int)dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
        randVel.y = (int)dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
        randVel.z = (int)dlb_rand32f_range(0.0f, METERS_TO_PIXELS(2.0f));
        particle.body.velocity = randVel;
        particle.body.friction = 0.5f;
    #else
        const float direction = 1.0f;
        float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
        particle.velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
    #endif
        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = { 154, 219, 63, 178 };  // Slime lime
        particle.sprite.scale = 1;
    }

    void goo_update(Particle &particle, float alpha)
    {
        const int radius = 5;
        // radius * 1.0 -> 0.2
        particle.sprite.scale = (int)(radius * (1.0f - alpha * 0.8f));
    }
}