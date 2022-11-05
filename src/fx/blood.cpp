#include "../particles.h"
#include "dlb_rand.h"

namespace FX {
    void blood_init(Particle &particle)
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

        //float vx = dlb_rand32f_variance(1.0f);
        //float vy = dlb_rand32f_variance(1.0f);
        //Vector2 vxy = v2_scale(v2_normalize({ vx, vy }), dlb_rand32f_range(0.3f, 1.0f));
        //vx = METERS_TO_PIXELS(vxy.x);
        //vy = METERS_TO_PIXELS(vxy.y);
        //vz = METERS_TO_PIXELS(2.0f);
        float vx = METERS_TO_PIXELS(dlb_rand32f_range(par.velocityXMin, par.velocityXMax));
        float vy = METERS_TO_PIXELS(dlb_rand32f_range(par.velocityYMin, par.velocityYMax));
        float vz = METERS_TO_PIXELS(dlb_rand32f_range(par.velocityZMin, par.velocityZMax));
        particle.body.velocity = { vx, vy, vz };
        particle.body.friction = par.friction;

        //particle.color = RED;
        const float spawnDelayAlpha = spawnDelay / (par.spawnDelayMax - par.spawnDelayMin);
        particle.sprite.scale = par.spawnScaleFirst + (par.spawnScaleLast - par.spawnScaleFirst) * spawnDelayAlpha;
        //particle.sprite.scale = MAX(1.0f, dlb_rand32f_range(1.0f, 8.0f) * (1.0f - ((float)alpha / spawnDelayMax)));
    }

    void blood_update(Particle &particle, float alpha)
    {
        const float radius = 5.0f;
        UNUSED(radius);
        // radius * 1.0 -> 0.2
        //particle.sprite.scale = radius * (1.0f - alpha);
        // 1.0 -> 0.0
        const unsigned char r = (unsigned char)((1.0f - alpha * 1.0f) * 255.0f);
        // 1.0 -> 0.1
        const unsigned char a = (unsigned char)((1.0f - alpha * 0.9f) * 255.0f);
        particle.color = { r, 0, 0, a };
    }
}