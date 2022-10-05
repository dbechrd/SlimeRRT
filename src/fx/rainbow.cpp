#include "../particles.h"
#include "dlb_rand.h"
#include <cassert>

namespace FX {
    void rainbow_init(Particle &particle)
    {
        assert(particle.effect);
        const ParticleEffect &effect = *particle.effect;

        const float angleRad = dlb_rand32f_range(0.0f, PI);
        Vector3 offset{};
        offset.x = -cosf(angleRad);
        offset.z = sinf(angleRad);

        // Spawn during first 50% of duration
        particle.spawnAt = effect.startedAt + (double)(angleRad / PI) * (effect.duration * 0.5);

        // Die randomly during last 15% of animation
        particle.dieAt = particle.spawnAt + (effect.duration * 0.4);
        assert(particle.dieAt > particle.spawnAt);

        const int colorArc = dlb_rand32i_range(0, 5);
        offset = v3_scale(offset, METERS_TO_PIXELS(3.0f) + (float)colorArc * METERS_TO_PIXELS(0.16f));
        particle.body.Teleport(offset);
        //particle.body.velocity = offset;
        particle.body.drag = 0.5f;
        particle.body.gravityScale = 0.0f; //-METERS_TO_PIXELS(0.002f);
        const float floatUpOffset = METERS_TO_PIXELS(dlb_rand32f_range(0.2f, 0.4f));
        particle.body.velocity.z = floatUpOffset;

        switch (colorArc) {
            case 0: particle.color = RED;        break;
            case 1: particle.color = ORANGE;     break;
            case 2: particle.color = GOLD;       break;
            case 3: particle.color = DARKGREEN;  break;
            case 4: particle.color = DARKBLUE;   break;
            case 5: particle.color = DARKPURPLE; break;
            default: particle.color = MAGENTA;
        }
        particle.sprite.scale = 1.0f;
    }

    void rainbow_update(Particle &particle, float alpha)
    {
        const float radius = 7.0f;
        // radius * 1.0 -> 0.2
        particle.sprite.scale = radius * (1.0f - alpha);

        // 1.0 -> 0.6
        particle.color.a = (unsigned char)((1.0f - alpha * 0.4f) * 255.0f);

        //const float floatUpOffset = dlb_rand32f_range(0.0f, METERS_TO_PIXELS(0.01f));
        //particle.body.Move3D({ 0.0f, 0.0f, floatUpOffset });
    }
}