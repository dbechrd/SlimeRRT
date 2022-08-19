#include "../particles.h"
#include "dlb_rand.h"
#include <cassert>

namespace FX {
    void number_init(Particle &particle)
    {
        assert(particle.effect);
        const ParticleEffect &effect = *particle.effect;
        const ParticleEffectParams &par = effect.params;

        // Spawn randomly during first 5% of duration
        particle.spawnAt = effect.startedAt + effect.duration * dlb_rand32f_variance(0.05f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.startedAt + effect.duration - (effect.duration * dlb_rand32f_variance(0.15f));
        assert(particle.dieAt > particle.spawnAt);

        float px = METERS_TO_PIXELS(dlb_rand32f_variance(0.2f));
        float py = METERS_TO_PIXELS(dlb_rand32f_variance(0.0f));
        float vz = METERS_TO_PIXELS(dlb_rand32f_range(1.0f, 2.0f));
        particle.body.Teleport({ px, py, METERS_TO_PIXELS(1.0f) });
        particle.body.velocity = { 0, 0, vz };
        particle.body.restitution = 0.95f;
        particle.body.friction = 0.1f;
        particle.body.gravityScale = 0.0f;

        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = WHITE;
        particle.sprite.scale = 2.0f;

        // TODO: Don't play particle effects on the server so that we can re-enable this assert on client side
        //assert(coinSpriteDef);
    }

    void number_update(Particle &particle, float alpha)
    {
        UNUSED(particle);
        UNUSED(alpha);
        assert(!!"foobar");
    }
}