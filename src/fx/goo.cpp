#include "particles.h"
#include "dlb_rand.h"
#include <cassert>

namespace FX {
    void goo_init(Particle &particle, ParticleEffect &effect)
    {
        // Spawn randomly during first 25% of duration
        particle.spawnAt = 0; //effect.duration * dlb_rand32f_variance(0.25f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.duration - (effect.duration * dlb_rand32f_variance(0.15f));
        assert(particle.dieAt > particle.spawnAt);

    #if 1
        Vector3i randVel {};
        randVel.x = dlb_rand32i_variance(METERS_TO_UNITS(1));
        randVel.y = dlb_rand32i_variance(METERS_TO_UNITS(1));
        randVel.z = dlb_rand32i_range(0, METERS_TO_UNITS(2));
        particle.body.velocity = randVel;
        particle.body.friction = 0.5f;
    #else
        const float direction = 1.0f;
        float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
        particle.velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
    #endif
        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = { 154, 219, 63, 178 };  // Slime lime
        particle.sprite.scale = 10;
    }

    void goo_update(Particle &particle, float alpha)
    {
        const int radius = 10;
        // radius * 1.0 -> 0.2
        particle.sprite.scale = (int)(radius * (1.0f - alpha * 0.8f));
    }
}