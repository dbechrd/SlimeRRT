#include "../particles.h"
#include "dlb_rand.h"

namespace FX {
    void poison_nova_init(Particle &particle)
    {
        DLB_ASSERT(particle.effect);
        const ParticleEffect &effect = *particle.effect;
        const ParticleEffectParams &par = effect.params;

        DLB_ASSERT(par.spawnDelayMin >= 0);
        DLB_ASSERT(par.spawnDelayMax >= par.spawnDelayMin);
        DLB_ASSERT(par.lifespanMin >= 0);
        DLB_ASSERT(par.lifespanMax >= par.lifespanMin);
        DLB_ASSERT(par.spawnScaleFirst > 0);
        DLB_ASSERT(par.spawnScaleLast > 0);

        const float spawnDelay = dlb_rand32f_range(par.spawnDelayMin, par.spawnDelayMax);
        const float lifetime = dlb_rand32f_range(par.lifespanMin, par.lifespanMax);

        particle.spawnAt = effect.startedAt + (double)spawnDelay;
        particle.dieAt = particle.spawnAt + (double)lifetime;
        DLB_ASSERT(particle.spawnAt >= effect.startedAt);
        DLB_ASSERT(particle.dieAt > particle.spawnAt);

        float vx = dlb_rand32f_variance(1.0f);
        float vy = dlb_rand32f_variance(1.0f);
        Vector2 vxy = v2_scale(v2_normalize({ vx, vy }), dlb_rand32f_range(par.velocityXMin, par.velocityXMax));
        vx = METERS_TO_PIXELS(vxy.x);
        vy = METERS_TO_PIXELS(vxy.y);
        //vz = METERS_TO_PIXELS(2.0f);
        //float vx = METERS_TO_PIXELS(dlb_rand32f_range(par.velocityXMin, par.velocityXMax));
        //float vy = METERS_TO_PIXELS(dlb_rand32f_range(par.velocityYMin, par.velocityYMax));
        float vz = METERS_TO_PIXELS(dlb_rand32f_range(par.velocityZMin, par.velocityZMax));
        particle.body.velocity = { vx, vy, vz };
        particle.body.friction = par.friction;

        //particle.color = RED;
        //const float spawnDelayAlpha = spawnDelay / (par.spawnDelayMax - par.spawnDelayMin);
        //particle.sprite.scale = par.spawnScaleFirst + (par.spawnScaleLast - par.spawnScaleFirst) * spawnDelayAlpha;
        //particle.sprite.scale = MAX(1.0f, dlb_rand32f_range(1.0f, 8.0f) * (1.0f - ((float)alpha / spawnDelayMax)));
    }

    void poison_nova_update(Particle &particle, float alpha)
    {
        const ParticleEffect &effect = *particle.effect;
        const ParticleEffectParams &par = effect.params;

        particle.sprite.scale = LERP(par.scaleA, par.scaleB, alpha);
        const unsigned char r = (unsigned char)LERP(64.0f, 255.0f, alpha);

        const float ha = alpha - 0.5f;
        const float a = CLAMP(ha*ha*ha*ha*ha*ha - 5*ha*ha + 1.0f, 0.0f, 1.0f);
        particle.color = { r, 0, 255, (unsigned char)(a * 255.0f) };
    }
}