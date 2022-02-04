#include "particles.h"
#include "dlb_rand.h"
#include <cassert>

namespace FX {
    void blood_init(Particle &particle, ParticleEffect &effect)
    {
        // Spawn randomly during first 25% of duration
        particle.spawnAt = effect.duration * dlb_rand32f_range(0.0f, 0.45f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.duration * dlb_rand32f_range(0.85f, 1.0f);
        assert(particle.dieAt > particle.spawnAt);

    #if 1
        Vector2 randXY = {};
        randXY.x = dlb_rand32f_variance(1.0f);
        randXY.y = dlb_rand32f_variance(1.0f);
        randXY = v2_scale(v2_normalize(randXY), METERS_TO_PIXELS(1.0f) * dlb_rand32f_range(0.0f, 3.0f));
        float randZ = dlb_rand32f_range(0.6f, METERS_TO_PIXELS(0.9f));
        particle.body.velocity = { randXY.x, randXY.y, randZ };
        particle.body.friction = 0.5f;
    #else
        const float direction = 1.0f;
        float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
        particle.velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
    #endif
        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = RED;
        particle.sprite.scale = 1.0f;
    }

    void blood_update(Particle &particle, float alpha)
    {
        const float radius = 5.0f;
        // radius * 1.0 -> 0.2
        particle.sprite.scale = radius * (1.0f - alpha);
        // 1.0 -> 0.0
        const unsigned char r = (unsigned char)((1.0f - alpha * 1.0f) * 255.0f);
        // 1.0 -> 0.6
        const unsigned char a = (unsigned char)((1.0f - alpha * 0.4f) * 255.0f);
        particle.color = { r, 0, 0, a };
    }
}