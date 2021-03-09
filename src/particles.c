#include "particles.h"
#include "helpers.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// NOTE: This defines 1 meter = 64 pixels
#define METERS(m) ((m) * 64.0f)

static void blood_generate(Particle *particle, float duration)
{
    // Spawn randomly during first 25% of duration
    particle->spawnAt = random_normalized(100) * duration * 0.25f;

    // Die randomly during last 15% of animation
    particle->dieAt = duration - random_normalized(100) * duration * 0.15f;
    assert(particle->dieAt > particle->spawnAt);

    particle->body.acceleration = (Vector3){ 0.0f, 0.0f, METERS(-10.0f) };  // gravity
#if 1
    float randX = random_normalized_signed(100) * METERS(1.0f);
    float randY = random_normalized_signed(100) * METERS(1.0f);
    float randZ = random_normalized(100) * METERS(4.0f);
    particle->body.velocity = (Vector3){ randX, randY, randZ };
    particle->body.friction = 1.0f;
#else
    const float direction = 1.0f;
    float randX = direction * random_normalized(100) * METERS(1.0f);
    particle->velocity = (Vector2){ randX, METERS(-3.0f) };
#endif
    //particle->position = (Vector2){ 0.0f, 0.0f };
    particle->scale = 1.0f;
    particle->color = RED;
}

static void blood_update(Particle *particle, float alpha)
{
    // 1.0 -> 0.2
    particle->scale = 1.0f - alpha * 0.8f;
    // 1.0 -> 0.0
    const unsigned char r = (unsigned char)((1.0f - alpha * 1.0f) * 255.0f);
    // 1.0 -> 0.6
    const unsigned char a = (unsigned char)((1.0f - alpha * 0.4f) * 255.0f);
    particle->color = (Color){ r, 0, 0, a };
}

static void blood_draw(Particle *particle)
{
    const float radius = 5.0f;
    DrawCircle(
        (int)particle->body.position.x,
        (int)(particle->body.position.y - particle->body.position.z),
        radius * particle->scale,
        particle->color
    );
}

static void gold_generate(Particle *particle, float duration)
{
    // Spawn randomly during first 25% of duration
    particle->spawnAt = random_normalized(100) * duration * 0.25f;

    // Die randomly during last 15% of animation
    particle->dieAt = duration - random_normalized(100) * duration * 0.15f;
    assert(particle->dieAt > particle->spawnAt);

    particle->body.acceleration = (Vector3){ 0.0f, 0.0f, METERS(-10.0f) };  // gravity
#if 1
    float randX = random_normalized_signed(100) * METERS(1.0f);
    float randY = random_normalized_signed(100) * METERS(1.0f);
    float randZ = random_normalized(100) * METERS(4.0f);
    particle->body.velocity = (Vector3){ randX, randY, randZ };
    particle->body.restitution = 0.8f;
    particle->body.friction = 0.5f;
#else
    const float direction = 1.0f;
    float randX = direction * random_normalized(100) * METERS(1.0f);
    particle->velocity = (Vector2){ randX, METERS(-3.0f) };
#endif
    //particle->position = (Vector2){ 0.0f, 0.0f };
    particle->scale = 0.5f;
    particle->color = YELLOW;
}

static void gold_update(Particle *particle, float time)
{
    // UNUSED for now
}

static void gold_draw(Particle *particle)
{
    const float radius = 12.0f;
    DrawCircle(
        (int)particle->body.position.x,
        (int)(particle->body.position.y - particle->body.position.z),
        radius * particle->scale,
        particle->color
    );
}

static void effect_generate(ParticleEffect *particleEffect)
{
    assert(particleEffect);

    Particle *particle = particleEffect->particles;
    void *particles_end = particleEffect->particles + particleEffect->particleCount;
    while (particle != particles_end) {
        particle->state = ParticleState_Dead;
        switch (particleEffect->type) {
            case ParticleEffectType_Blood: {
                blood_generate(particle, (float)particleEffect->duration);
                break;
            } case ParticleEffectType_Gold: {
                gold_generate(particle, (float)particleEffect->duration);
                break;
            } default: {
                TraceLog(LOG_FATAL, "Unknown particle effect type\n");
                return;
            }
        }
        particle++;
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
    effect_generate(particleEffect);
}

bool particle_effect_start(ParticleEffect *particleEffect, double time, Vector3 origin)
{
    assert(particleEffect);
    assert(time);

    if (particleEffect->state == ParticleEffectState_Dead) {
        particleEffect->state = ParticleEffectState_Alive;
        particleEffect->origin = origin;
        particleEffect->startedAt = time;
        particleEffect->lastUpdatedAt = time;
        effect_generate(particleEffect);
        return true;
    }
    return false;
}

static void particle_effect_reset(ParticleEffect *particleEffect)
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

void particle_effect_stop(ParticleEffect *particleEffect)
{
    assert(particleEffect);

    if (particleEffect->state == ParticleEffectState_Dead) {
        return;
    }

    if (particleEffect->callbacks[ParticleEffectEvent_Stopping]) {
        particleEffect->callbacks[ParticleEffectEvent_Stopping](particleEffect);
    }

    particle_effect_reset(particleEffect);
}

void particle_effect_update(ParticleEffect *particleEffect, double time)
{
    assert(particleEffect);
    assert(time);

    if (particleEffect->state == ParticleEffectState_Dead) {
        return;
    }

    const float dt       = (float)(time - particleEffect->lastUpdatedAt);
    const float animTime = (float)(time - particleEffect->startedAt);

    size_t deadParticleCount = 0;
    Particle *particle = particleEffect->particles;
    void *particles_end = particleEffect->particles + particleEffect->particleCount;

    while (particle != particles_end) {
        const float alpha = (float)((animTime - particle->spawnAt) / (particle->dieAt - particle->spawnAt));
        if (alpha >= 0.0f && alpha < 1.0f) {
            if (particle->state == ParticleState_Dead) {
                particle->body.position = particleEffect->origin;
                particle->state = ParticleState_Alive;
            }
            body3d_update(&particle->body, dt);

            switch (particleEffect->type) {
                case ParticleEffectType_Blood: {
                    blood_update(particle, alpha);
                    break;
                } case ParticleEffectType_Gold: {
                    gold_update(particle, alpha);
                    break;
                } default: {
                    TraceLog(LOG_FATAL, "Unknown particle effect type\n");
                    return;
                }
            }
        } else if(alpha >= 1.0f) {
            particle->state = ParticleState_Dead;
            deadParticleCount++;
        }
        particle++;
    }

    if (deadParticleCount == particleEffect->particleCount) {
        if (particleEffect->callbacks[ParticleEffectEvent_Dying]) {
            particleEffect->callbacks[ParticleEffectEvent_Dying](particleEffect);
        }
        particle_effect_reset(particleEffect);
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
    while (particle != particles_end) {
        if (particle->state == ParticleState_Alive) {
            switch (particleEffect->type) {
                case ParticleEffectType_Blood: {
                    blood_draw(particle);
                    break;
                } case ParticleEffectType_Gold: {
                    gold_draw(particle);
                    break;
                } default: {
                    TraceLog(LOG_FATAL, "Unknown particle effect type\n");
                    return;
                }
            }
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