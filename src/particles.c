#include "particles.h"
#include "helpers.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t activeEffectsCapacity;
static ParticleEffect **activeEffects;

static void blood_generate(Particle *particle, float duration)
{
    // Spawn randomly during first 25% of duration
    particle->spawnAt = random_normalized(100) * duration * 0.25f;

    // Die randomly during last 15% of animation
    particle->dieAt = duration - random_normalized(100) * duration * 0.15f;
    assert(particle->dieAt > particle->spawnAt);

#if 1
    float randX = random_normalized_signed(100) * METERS(1.0f);
    float randY = random_normalized_signed(100) * METERS(1.0f);
    float randZ = random_normalized(100) * METERS(2.0f);
    particle->body.velocity = (Vector3){ randX, randY, randZ };
    particle->body.friction = 0.5f;
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
    particle->spawnAt = random_normalized(100) * duration * 0.05f;

    // Die randomly during last 15% of animation
    particle->dieAt = duration - random_normalized(100) * duration * 0.15f;
    assert(particle->dieAt > particle->spawnAt);

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
    particle->scale = 1.0f;
    particle->color = YELLOW;
}

static void gold_update(Particle *particle, float alpha)
{
    // UNUSED for now
}

static void gold_draw(Particle *particle)
{
    const float radius = 10.0f;
    DrawCircle(
        (int)particle->body.position.x,
        (int)(particle->body.position.y - particle->body.position.z),
        radius * particle->scale,
        particle->color
    );
}

static void effect_generate(ParticleEffect *effect)
{
    assert(effect);

    for (size_t i = 0; i < effect->particleCount; i++) {
        Particle *particle = &effect->particles[i];
        particle->state = ParticleState_Dead;
        switch (effect->type) {
            case ParticleEffectType_Blood: {
                blood_generate(particle, (float)effect->duration);
                break;
            } case ParticleEffectType_Gold: {
                gold_generate(particle, (float)effect->duration);
                break;
            } default: {
                TraceLog(LOG_FATAL, "Unknown particle effect type\n");
                return;
            }
        }
    }
}

ParticleEffect *particle_effect_alloc(ParticleEffectType type, size_t particleCount)
{
    assert(type >= 0);
    assert(type < ParticleEffectType_Count);
    assert(particleCount);

    // Allocate effect
    ParticleEffect *effect = calloc(1, sizeof(*effect) + sizeof(*effect->particles) * particleCount);
    assert(effect);

    effect->type = type;
    effect->particleCount = particleCount;
    effect->particles = (Particle *)((char *)effect + sizeof(*effect));

    int freeIndex = -1;

    // Find free slot for effect pointer
    for (size_t i = 0; i < activeEffectsCapacity; i++) {
        if (activeEffects[i] == NULL) {
            freeIndex = (int)i;
            break;
        }
    }

    // If not found, expand array
    if (freeIndex == -1) {
        if (!activeEffects) {
            activeEffectsCapacity = 16;
            activeEffects = calloc(activeEffectsCapacity, sizeof(*activeEffects));
            freeIndex = 0;
        } else {
            activeEffects = realloc(activeEffects, sizeof(*activeEffects) * activeEffectsCapacity * 2);
            freeIndex = (int)activeEffectsCapacity;
            activeEffectsCapacity *= 2;
        }
        assert(activeEffects);
    }

    // Store pointer to new effect in effects list
    assert(freeIndex >= 0);
    assert(freeIndex < activeEffectsCapacity);
    activeEffects[freeIndex] = effect;
    return effect;
}

bool particle_effect_start(ParticleEffect *effect, double now, double duration, Vector3 origin)
{
    assert(effect);
    assert(duration > 0.0);

    if (effect->state == ParticleEffectState_Dead) {
        effect->state = ParticleEffectState_Alive;
        effect->origin = origin;
        effect->startedAt = now;
        effect->duration = duration;
        effect_generate(effect);
        return true;
    }
    return false;
}
//
//static void particle_effect_reset(ParticleEffect *effect)
//{
//    assert(effect);
//    assert(effect->particles);
//
//    effect->startedAt = 0;
//    effect->lastUpdatedAt = 0;
//    effect->state = ParticleEffectState_Dead;
//    memset(effect->particles, 0, sizeof(*effect->particles) * effect->particleCount);
//}

void particle_effect_stop(ParticleEffect *effect)
{
    assert(effect);

    if (effect->state == ParticleEffectState_Dead) {
        return;
    }

    effect->state = ParticleEffectState_Stopping;
    ParticeEffectEventCallback *callback = &effect->callbacks[ParticleEffectEvent_Stopped];
    if (callback->function) {
        callback->function(effect, callback->userData);
    }
    particle_effect_free(effect);
}

static void particle_effect_update(ParticleEffect *effect, double now, double dt)
{
    assert(effect);
    assert(effect->particleCount);
    assert(effect->particles);

    if (effect->state == ParticleEffectState_Dead) {
        return;
    }

    ParticeEffectEventCallback *callback = &effect->callbacks[ParticleEffectEvent_BeforeUpdate];
    if (callback->function) {
        callback->function(effect, callback->userData);
    }

    const float animTime = (float)(now - effect->startedAt);

    size_t deadParticleCount = 0;

    for (size_t i = 0; i < effect->particleCount; i++) {
        Particle *particle = &effect->particles[i];
        const float alpha = (float)((animTime - particle->spawnAt) / (particle->dieAt - particle->spawnAt));
        if (alpha >= 0.0f && alpha < 1.0f) {
            if (particle->state == ParticleState_Dead) {
                particle->body.position = effect->origin;
                particle->state = ParticleState_Alive;
            }
            body_update(&particle->body, now, dt);

            switch (effect->type) {
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
    }

    if (deadParticleCount == effect->particleCount) {
        ParticeEffectEventCallback *callback = &effect->callbacks[ParticleEffectEvent_Dying];
        if (callback->function) {
            callback->function(effect, callback->userData);
        }
        particle_effect_free(effect);
    }
}

void particle_effects_update(double now, double dt)
{
    for (size_t i = 0; i < activeEffectsCapacity; i++) {
        if (activeEffects[i]) {
            particle_effect_update(activeEffects[i], now, dt);
        }
    }
}

static void particle_effect_draw(ParticleEffect *effect)
{
    assert(effect);

    if (effect->state == ParticleEffectState_Dead) {
        return;
    }

    for (size_t i = 0; i < effect->particleCount; i++) {
        Particle *particle = &effect->particles[i];
        if (particle->state == ParticleState_Alive) {
            switch (effect->type) {
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
    }
}

void particle_effects_draw()
{
    for (size_t i = 0; i < activeEffectsCapacity; i++) {
        if (activeEffects[i]) {
            particle_effect_draw(activeEffects[i]);
        }
    }
}

void particle_effect_free(ParticleEffect *effect)
{
    assert(effect);

    // Remove effect pointer from active effects list
    for (size_t i = 0; i < activeEffectsCapacity; i++) {
        if (activeEffects[i] == effect) {
            activeEffects[i] = NULL;
        }
    }

    free(effect);
}

void particle_effects_free()
{
    // Remove effect pointer from active effects list
    for (size_t i = 0; i < activeEffectsCapacity; i++) {
        if (activeEffects[i]) {
            particle_effect_free(activeEffects[i]);
        }
    }

    free(activeEffects);
}