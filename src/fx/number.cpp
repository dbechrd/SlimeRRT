#include "../particles.h"
#include "dlb_rand.h"

namespace FX {
    void number_init(Particle &particle)
    {
        DLB_ASSERT(particle.effect);
        const ParticleEffect &effect = *particle.effect;

        // Spawn randomly during first 5% of duration
        particle.spawnAt = effect.startedAt + effect.duration * (double)dlb_rand32f_variance(0.05f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.startedAt + effect.duration - (effect.duration * (double)dlb_rand32f_variance(0.15f));
        DLB_ASSERT(particle.dieAt > particle.spawnAt);

        float vx = METERS_TO_PIXELS(dlb_rand32f_variance(0.5f));
        float vy = METERS_TO_PIXELS(dlb_rand32f_variance(0.5f));
        float vz = METERS_TO_PIXELS(dlb_rand32f_range(4.0f, 5.0f));
        particle.body.Teleport({ vx, vy, 0 });
        particle.body.velocity = { 0, 0, vz };
        particle.body.restitution = 0.5f;
        particle.body.friction = 0.5f;
        particle.body.gravityScale = 1.3f;

        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = WHITE;
        particle.sprite.scale = 2.0f;

        // TODO: Don't play particle effects on the server so that we can re-enable this assert on client side
        //DLB_ASSERT(coinSpriteDef);
    }

    void number_update(Particle &particle, float alpha)
    {
        UNUSED(particle);
        UNUSED(alpha);
    }
}