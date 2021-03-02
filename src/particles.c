#include "particles.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// NOTE: This defines 1 meter = 64 pixels
#define METERS(m) ((m) * 64.0f)

// Generate random number in [0.0f, 1.0f] range, with specified resolution (higher = smaller increments in variability)
static float random_normalized(int resolution)
{
    float value = GetRandomValue(0, resolution) / (float)resolution;
    return value;
}

// Generate random number in [-1.0f, 1.0f] range, with specified resolution (higher = smaller increments in variability)
static float random_normalized_signed(int resolution)
{
    float value = GetRandomValue(0, resolution) / (float)resolution * 2.0f - 1.0f;
    return value;
}

static void particle_effect_generate_blood(ParticleEffect *particleEffect)
{
    Particle *particle = particleEffect->particles;
    void *particles_end = particleEffect->particles + particleEffect->particleCount;
    while (particle != particles_end) {
        particle->state = ParticleState_Dead;

        // Spawn randomly during first 25% of duration
        particle->spawnAt = random_normalized(100) * particleEffect->duration * 0.25f;

        // Die randomly during last 15% of animation
        particle->dieAt = particleEffect->duration - random_normalized(100) * (particleEffect->duration * 0.15f);
        assert(particle->dieAt > particle->spawnAt);

        particle->acceleration = (Vector2){ 0.0f, METERS(10.0f) };  // gravity
#if 1
        float randX = random_normalized_signed(100) * METERS(1.0f);
        particle->velocity = (Vector2){ randX, METERS(-4.0f) };
#else
        const float direction = 1.0f;
        float randX = direction * random_normalized(100) * METERS(1.0f);
        particle->velocity = (Vector2){ randX, METERS(-3.0f) };
#endif
        //particle->position = (Vector2){ 0.0f, 0.0f };
        particle->scale = 1.0f;
        particle->color = RED;
        particle++;
    }
}

static void particle_effect_update_blood(ParticleEffect *particleEffect, double time)
{
    const float dt       = (float)(time - particleEffect->lastUpdatedAt);
    const float animTime = (float)(time - particleEffect->startedAt);

    size_t deadParticleCount = 0;
    Particle *particle = particleEffect->particles;
    void *particles_end = particleEffect->particles + particleEffect->particleCount;

    while (particle != particles_end) {
        const float alpha = (float)((animTime - particle->spawnAt) / (particle->dieAt - particle->spawnAt));
        if (alpha >= 0.0f && alpha < 1.0f) {
            if (particle->state == ParticleState_Dead) {
                particle->position = particleEffect->origin;
                particle->state = ParticleState_Alive;
            }
            particle->velocity.x += particle->acceleration.x * dt;
            particle->velocity.y += particle->acceleration.y * dt;
            particle->position.x += particle->velocity.x * dt;
            particle->position.y += particle->velocity.y * dt;
            // 1.0 -> 0.2
            particle->scale = 1.0f - alpha * 0.8f;
            // 1.0 -> 0.0
            const unsigned char r = (unsigned char)((1.0f - alpha * 1.0f) * 255.0f);
            // 1.0 -> 0.6
            const unsigned char a = (unsigned char)((1.0f - alpha * 0.4f) * 255.0f);
            particle->color = (Color){ r, 0, 0, a };
        } else if(alpha >= 1.0f) {
            particle->state = ParticleState_Dead;
            deadParticleCount++;
        }
        particle++;
    }

    if (deadParticleCount == particleEffect->particleCount) {
        particle_effect_stop(particleEffect);
    }
}

void particle_effect_generate(ParticleEffect *particleEffect, ParticleEffectType type, size_t particleCount, double duration)
{
    assert(particleEffect);
    assert(!particleEffect->particles);  // double initialization error; use particle_effect_reset() to reuse the buffer
    assert(particleCount);
    assert(type >= 0);
    assert(type < ParticleEffectType_Count);
    assert(duration >= 0.0);

    Particle *particles = calloc(particleCount, sizeof(*particleEffect->particles));
    if (!particles) {
        TraceLog(LOG_FATAL, "Failed to allocate particle effect\n");
        return;
    }
    particleEffect->type = type;
    particleEffect->state = ParticleEffectState_Dead;
    particleEffect->startedAt = 0;
    particleEffect->duration = duration;
    particleEffect->particleNext = 0;
    particleEffect->particleCount = particleCount;
    particleEffect->particles = particles;

    switch (type) {
        case ParticleEffectType_Blood: {
            particle_effect_generate_blood(particleEffect);
            break;
        } default: {
            TraceLog(LOG_FATAL, "Unknown particle effect type\n");
            free(particleEffect->particles);
            return;
        }
    }
}

bool particle_effect_start(ParticleEffect *particleEffect, double time, Vector2 origin)
{
    assert(particleEffect);
    assert(time);

    if (particleEffect->state == ParticleEffectState_Dead) {
        particleEffect->state = ParticleEffectState_Alive;
        particleEffect->origin = origin;
        particleEffect->startedAt = time;
        particleEffect->lastUpdatedAt = time;
        particle_effect_generate_blood(particleEffect);
        return true;
    }
    return false;
}

void particle_effect_stop(ParticleEffect *particleEffect)
{
    assert(particleEffect);
    assert(particleEffect->particles);
    assert(particleEffect->particleCount);

    particleEffect->particleNext = 0;
    memset(particleEffect->particles, 0, particleEffect->particleCount * sizeof(*particleEffect->particles));
    particleEffect->startedAt = 0;
    particleEffect->lastUpdatedAt = 0;
    particleEffect->state = ParticleEffectState_Dead;
}

void particle_effect_update(ParticleEffect *particleEffect, double time)
{
    assert(particleEffect);
    assert(time);

    if (particleEffect->state == ParticleEffectState_Dead) {
        return;
    }

    switch (particleEffect->type) {
        case ParticleEffectType_Blood: {
            particle_effect_update_blood(particleEffect, time);
            break;
        } default: {
            TraceLog(LOG_FATAL, "Unknown particle effect type\n");
            free(particleEffect->particles);
            return;
        }
    }

    particleEffect->lastUpdatedAt = time;
}

void particle_effect_draw(ParticleEffect *particleEffect, double time)
{
    assert(particleEffect);

    if (particleEffect->state == ParticleEffectState_Dead) {
        return;
    }

    const float age = (float)(time - particleEffect->startedAt);

    Particle *particle = particleEffect->particles;
    void *particles_end = particleEffect->particles + particleEffect->particleCount;
    const float radius = 5.0f;
    while (particle != particles_end) {
        if (particle->state == ParticleState_Alive) {
            DrawCircle(
                (int)particle->position.x,
                (int)particle->position.y,
                radius * particle->scale,
                particle->color
            );
        }
        particle++;
    }
}

void particle_effect_free(ParticleEffect *particleEffect)
{
    assert(particleEffect);

    free(particleEffect->particles);
    memset(particleEffect, 0, sizeof(*particleEffect));
}