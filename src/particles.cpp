#include "particles.h"
#include "particle_fx.h"
#include "sprite.h"
#include "spritesheet.h"
#include "helpers.h"
#include "draw_command.h"
#include "dlb_rand.h"
#include "chat.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

#define MAX_EFFECTS   32

static Particle *particles;
static Particle *particlesFree;
static size_t particlesActiveCount;
static ParticleEffect *effects;
static ParticleEffect *effectsFree;
static size_t effectsActiveCount;

void particles_init(void)
{
    assert(MAX_PARTICLES);
    assert(MAX_EFFECTS);

    // Allocate pools
    particles = (Particle *)calloc(MAX_PARTICLES, sizeof(*particles));
    effects = (ParticleEffect *)calloc(MAX_EFFECTS, sizeof(*effects));
    if (!particles || !effects) {
        TraceLog(LOG_FATAL, "Failed to allocate memory for particles\n");
        exit(-1);
    }

    ChatMessage *a = (ChatMessage *)calloc(CHAT_MESSAGE_HISTORY, sizeof(*a));
    for (size_t i = 0; i < CHAT_MESSAGE_HISTORY; i++) {
        new(a + i) ChatMessage{};
    }

    // Initialze intrusive free lists
    for (size_t i = 0; i < MAX_PARTICLES - 1; i++) {
        new(particles + i) Particle{};
        particles[i].next = &particles[i + 1];
    }
    particlesFree = particles;
    for (size_t i = 0; i < MAX_EFFECTS - 1; i++) {
        new(effects + i) ParticleEffect{};
        effects[i].next = &effects[i + 1];
    }
    effectsFree = effects;
}

void particles_free(void)
{
    free(particles);
    free(effects);
    particles = 0;
    effects = 0;
    particlesFree = 0;
    effectsFree = 0;
}

static Particle *particle_alloc(void)
{
    // Allocate effect
    Particle *particle = particlesFree;
    if (!particle) {
        assert(!"Particle pool is full");
        TraceLog(LOG_ERROR, "Particle pool is full; discarding particle.\n");
        return 0;
    }
    particlesFree = particle->next;
    particlesActiveCount++;

    return particle;
}

static void generate_effect_particles(ParticleEffect *effect, size_t particleCount, const SpriteDef *spriteDef)
{
    Particle *prev = 0;
    for (size_t i = 0; i < particleCount; i++) {
        Particle *particle = particle_alloc();
        if (!particle) {
            break;
        }
        if (prev) {
            prev->next = particle;
        }

        particle->effect = effect;
        particle->sprite.spriteDef = spriteDef;
        effect->def->init(*particle, effect->duration);
        effect->particlesLeft++;

        prev = particle;
    }
}

ParticleEffect *particle_effect_create(ParticleEffectType type, size_t particleCount, Vector3 origin, double duration,
    double now, const SpriteDef *spriteDef)
{
    assert((int)type > 0);
    assert((int)type < (int)ParticleEffectType::Count);
    assert(particleCount);
    assert(duration);
    assert(duration > 0.0);

    // Allocate effect
    ParticleEffect *effect = effectsFree;
    if (!effect) {
        assert(!"Particle effect pool is full");
        TraceLog(LOG_ERROR, "Particle effect pool is full; discarding particle effect.\n");
        return 0;
    }
    effectsFree = effect->next;
    effectsActiveCount++;

    // Sanity checks to ensure previous effect was freed properly and/or free list is returning valid pointers
    assert(effect->type == ParticleEffectType::Dead);
    assert(effect->particlesLeft == 0);

    effect->type = type;
    effect->origin = origin;
    effect->duration = duration;
    effect->startedAt = now;

    static ParticleDef particle_fx_defs[(int)ParticleEffectType::Count]{};
    particle_fx_defs[(int)ParticleEffectType::Blood] = { particle_fx_blood_init, particle_fx_blood_update };
    particle_fx_defs[(int)ParticleEffectType::Gold ] = { particle_fx_gold_init, particle_fx_gold_update };
    particle_fx_defs[(int)ParticleEffectType::Goo  ] = { particle_fx_goo_init, particle_fx_goo_update };
    effect->def = &particle_fx_defs[(int)effect->type];

    generate_effect_particles(effect, particleCount, spriteDef);

    return effect;
}

size_t particles_active(void)
{
    return particlesActiveCount;
}

size_t particle_effects_active(void)
{
    return effectsActiveCount;
}

void particles_update(double now, double dt)
{
    assert(particlesActiveCount <= MAX_PARTICLES);
    assert(effectsActiveCount <= MAX_EFFECTS);

    size_t effectsCounted = 0;
    for (size_t i = 0; effectsCounted < effectsActiveCount; i++) {
        ParticleEffect &effect = effects[i];
        if (effect.type == ParticleEffectType::Dead)
            continue;

        if (effect.callbacks[(int)ParticleEffectEventType::BeforeUpdate].function) {
            effect.callbacks[(int)ParticleEffectEventType::BeforeUpdate].function(
                effect, effect.callbacks[(int)ParticleEffectEventType::BeforeUpdate].userData
            );
        }
        effectsCounted++;
    }

    size_t particlesCounted = 0;
    for (size_t i = 0; particlesCounted < particlesActiveCount; i++) {
        Particle &particle = particles[i];
        if (!particle.effect)
            continue;  // particle is dead

        ParticleEffect &effect = *particle.effect;
        const float animTime = (float)(now - effect.startedAt);
        const float alpha = (float)((animTime - particle.spawnAt) / (particle.dieAt - particle.spawnAt));
        if (alpha >= 0.0f && alpha < 1.0f) {
            if (!particle.body.lastUpdated) {
                particle.body.position = effect.origin;
            }
            particle.body.Update(now, dt);
            sprite_update(particle.sprite, now, dt);
            effect.def->update(particle, alpha);
        } else if (alpha >= 1.0f) {
            effect.particlesLeft--;

            // Return particle to free list
            particle = {};
            particle.next = particlesFree;
            particlesFree = &particle;
            particlesActiveCount--;
        }
        particlesCounted++;
    }

    effectsCounted = 0;
    for (size_t i = 0; effectsCounted < effectsActiveCount; i++) {
        ParticleEffect &effect = effects[i];
        if (effect.type == ParticleEffectType::Dead)
            continue;

        // note: ParticleEffectEvent_AfterUpdate would go here if I ever care about that..

        if (!effect.particlesLeft) {
            if (effect.callbacks[(int)ParticleEffectEventType::Dying].function) {
                effect.callbacks[(int)ParticleEffectEventType::Dying].function(
                    effect, effect.callbacks[(int)ParticleEffectEventType::Dying].userData
                );
            }

            // Return effect to free list
            effect = {};
            effect.next = effectsFree;
            effectsFree = &effect;
            effectsActiveCount--;
        }
        effectsCounted++;
    }
}

void particles_push(DrawList &drawList)
{
    assert(particlesActiveCount <= MAX_PARTICLES);

    size_t particlesPushed = 0;
    for (size_t i = 0; particlesPushed < particlesActiveCount; i++) {
        Particle *particle = &particles[i];
        if (!particle->effect)
            continue;  // particle is dead

        drawList.Push(*particle);
        particlesPushed++;
    }
}

float Particle::Depth() const
{
    const float depth = body.position.y;
    return depth;
}

bool Particle::Cull(const Rectangle &cullRect) const
{
    bool cull = false;

    if (sprite.spriteDef) {
        cull = sprite_cull_body(sprite, body, cullRect);
    }
    else {
        const Vector2 particleBC = body.BottomCenter();
        cull = !CheckCollisionCircleRec(particleBC, sprite.scale, cullRect);
    }

    return cull;
}

void Particle::Draw() const
{
    if (sprite.spriteDef) {
        sprite_draw_body(sprite, body, color);
    }
    else {
        DrawCircle(
            (int)body.position.x,
            (int)(body.position.y - body.position.z),
            sprite.scale,
            color
        );
    }
}