#include "particles.h"
#include "catalog/particle_fx.h"
#include "spritesheet.h"
#include "helpers.h"
#include "draw_command.h"
#include "dlb_rand.h"
#include "chat.h"
#include <cstdlib>
#include <cstring>

ParticleSystem::ParticleSystem(void)
{
    // Initialze intrusive free lists
    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        if (i < MAX_PARTICLES - 1) {
            particles[i].next = &particles[i + 1];
        }
    }
    particlesFree = particles;

    for (size_t i = 0; i < MAX_EFFECTS; i++) {
        if (i < MAX_EFFECTS - 1) {
            effects[i].next = &effects[i + 1];
        }
    }
    effectsFree = effects;
}

ParticleSystem::~ParticleSystem(void)
{
    //memset(particles, 0, sizeof(particles));
    //memset(effects, 0, sizeof(effects));
    //particlesFree = 0;
    //effectsFree = 0;
}

Particle *ParticleSystem::Alloc(void)
{
    // Allocate effect
    Particle *particle = particlesFree;
    if (!particle) {
        // TODO: Delete oldest particles instead of dropping newest ones?
        //DLB_ASSERT(!"Particle pool is full");
        //TraceLog(LOG_ERROR, "Particle pool is full; discarding particle.\n");
        return 0;
    }
    particlesFree = particle->next;
    particlesActiveCount++;

    return particle;
}

ParticleEffect *ParticleSystem::GenerateEffect(Catalog::ParticleEffectID type, Vector3 origin, const ParticleEffectParams &par)
{
    DLB_ASSERT((size_t)type > 0);
    DLB_ASSERT((size_t)type < (size_t)Catalog::ParticleEffectID::Count);
    DLB_ASSERT(par.particleCountMin > 0);
    DLB_ASSERT(par.particleCountMax >= par.particleCountMax);
    DLB_ASSERT(par.durationMin > 0.0f);
    DLB_ASSERT(par.durationMax >= par.durationMin);

    const Catalog::ParticleEffectDef &pfx = Catalog::g_particleFx.FindById(type);
    DLB_ASSERT(pfx.init);
    if (!pfx.init) {
        return 0;
    }

    // Allocate effect
    ParticleEffect *effect = effectsFree;
    if (!effect) {
        //DLB_ASSERT(!"Particle effect pool is full");
        //TraceLog(LOG_ERROR, "Particle effect pool is full; discarding particle effect.\n");
        return 0;
    }
    effectsFree = effect->next;
    effectsActiveCount++;

    // Sanity checks to ensure previous effect was freed properly and/or free list is returning valid pointers
    DLB_ASSERT(effect->id == Catalog::ParticleEffectID::Empty);
    DLB_ASSERT(effect->particlesLeft == 0);

    effect->id = type;
    effect->origin = origin;
    effect->duration = dlb_rand32f_range(par.durationMin, par.durationMax);
    effect->startedAt = g_clock.now;
    effect->params = par;

    const int particleCount = dlb_rand32i_range(par.particleCountMin, par.particleCountMax);

    Particle *prev = 0;
    for (int i = 0; i < particleCount; i++) {
        Particle *particle = Alloc();
        if (!particle) {
            break;
        }
        if (prev) {
            prev->next = particle;
        }

        particle->effect = effect;
        DLB_ASSERT(pfx.init);
        pfx.init(*particle);
        effect->particlesLeft++;

        prev = particle;
    }

    return effect;
}

size_t ParticleSystem::ParticlesActive(void)
{
    return particlesActiveCount;
}

size_t ParticleSystem::EffectsActive(void)
{
    return effectsActiveCount;
}

Particle *ParticleSystem::ParticlePool(void)
{
    return particles;
}

void ParticleSystem::Update(double dt)
{
    DLB_ASSERT(particlesActiveCount <= MAX_PARTICLES);
    DLB_ASSERT(effectsActiveCount <= MAX_EFFECTS);

    size_t effectsCounted = 0;
    for (size_t i = 0; effectsCounted < effectsActiveCount; i++) {
        ParticleEffect &effect = effects[i];
        if (effect.id == Catalog::ParticleEffectID::Empty)
            continue;

        const ParticleEffect_Callback &beforeUpdate = effect.effectCallbacks[(size_t)ParticleEffect_Event::BeforeUpdate];
        if (beforeUpdate.callback) {
            beforeUpdate.callback(effect, beforeUpdate.world, beforeUpdate.entityId);
        }
        effectsCounted++;
    }

    size_t particlesCounted = 0;
    for (size_t i = 0; particlesCounted < particlesActiveCount; i++) {
        Particle &particle = particles[i];
        if (!particle.effect)
            continue;  // particle is dead

        ParticleEffect &effect = *particle.effect;
        const float alpha = (float)((g_clock.now - particle.spawnAt) / (particle.dieAt - particle.spawnAt));
        if (alpha >= 0.0f && alpha < 1.0f) {
            if (!particle.alive) {
                Vector3 pos = particle.body.WorldPosition();
                particle.body.Teleport(effect.origin);
                particle.body.Move3D(pos);
                particle.alive = true;
            }
            particle.body.drag = effect.params.drag;
            particle.body.gravityScale = LERP(effect.params.gravityScaleA, effect.params.gravityScaleB, alpha);
            particle.body.Update(dt);
            sprite_update(particle.sprite, dt);
            DLB_ASSERT(Catalog::g_particleFx.FindById(effect.id).update);
            Catalog::g_particleFx.FindById(effect.id).update(particle, alpha);

            const ParticleEffect_ParticleCallback &afterUpdate = effect.particleCallbacks[(size_t)ParticleEffect_ParticleEvent::AfterUpdate];
            if (afterUpdate.callback) {
                afterUpdate.callback(particle, afterUpdate.world, afterUpdate.entityId);
            }
        } else if (alpha >= 1.0f) {
            effect.particlesLeft--;

            const ParticleEffect_ParticleCallback &died = effect.particleCallbacks[(size_t)ParticleEffect_ParticleEvent::Died];
            if (died.callback) {
                died.callback(particle, died.world, died.entityId);
            }

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
        if (effect.id == Catalog::ParticleEffectID::Empty)
            continue;

        // note: ParticleEffectEvent_AfterUpdate would go here if I ever care about that..

        if (!effect.particlesLeft) {
            const ParticleEffect_Callback &dying = effect.effectCallbacks[(size_t)ParticleEffect_Event::Dying];
            if (dying.callback) {
                dying.callback(effect, dying.world, dying.entityId);
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

void ParticleSystem::PushAll(DrawList &drawList)
{
    DLB_ASSERT(particlesActiveCount <= MAX_PARTICLES);

    for (size_t i = 0; i < MAX_PARTICLES; i++) {
        Particle &particle = particles[i];
        if (!particle.alive)
            continue;  // particle is dead

        //const float animTime = (float)(g_clock.now - particle.effect->startedAt);
        //const float alpha = (float)((animTime - particle.spawnAt) / (particle.dieAt - particle.spawnAt));
        //if (alpha >= 0.0f && alpha < 1.0f) {
        drawList.Push(particle, particle.Depth(), particle.Cull(drawList.cullRect));
    }
}

float Particle::Depth(void) const
{
    const float depth = body.GroundPosition().y;
    return depth;
}

bool Particle::Cull(const Rectangle &cullRect) const
{
    bool cull = false;
    if (sprite.spriteDef) {
        cull = sprite_cull_body(sprite, body, cullRect);
    } else {
        const Vector2 bodyBC = body.VisualPosition();
        cull = !CheckCollisionCircleRec(bodyBC, sprite.scale, cullRect);
    }
    return cull;
}

void Particle::Draw(World &world, Vector2 at)
{
    UNUSED(world);
    UNUSED(at);
    const ParticleEffect_ParticleCallback &draw = effect->particleCallbacks[(size_t)ParticleEffect_ParticleEvent::Draw];
    if (draw.callback) {
        draw.callback(const_cast<Particle &>(*this), draw.world, draw.entityId);
    } else {
        if (sprite.spriteDef) {
            sprite_draw_body(sprite, body, color);
        } else {
            const Vector3 pos = body.WorldPosition();
            const float halfW = sprite.scale / 2.0f;
            DrawRectangleRec({ pos.x - halfW, pos.y - pos.z - halfW, sprite.scale, sprite.scale }, color);
            //DrawCircleSector({ pos.x, pos.y - pos.z }, sprite.scale, 0.0f, 360.0f, 12, color);
            //DrawCircleSectorLines({ pos.x, pos.y - pos.z }, sprite.scale, 0.0f, 360.0f, 12, Fade(BLACK, color.a / 255.0f));
        }
    }
}

void ParticlesFollowPlayerGut(ParticleEffect &effect, World *world, EntityID entityId)
{
    DLB_ASSERT(world);
    DLB_ASSERT(entityId);

    Attach *attach = (Attach *)world->facetDepot.FacetFind(entityId, Facet_Attach);
    DLB_ASSERT(attach);
    if (attach) {
        effect.origin = attach->points[Attach_Gut];
    }
}

// HACK(cleanup): Using *EXTREME* HAXOR HAX to stuff a const char * into the World*
void ParticleDrawText(Particle &particle, World *world, EntityID entityId)
{
    DLB_ASSERT(world);
    const char *text = (const char *)world;
    UI::ParticleText(particle.body.VisualPosition(), text);
}

void ParticleFreeText(ParticleEffect &effect, World *world, EntityID entityId)
{
    UNUSED(effect);
    DLB_ASSERT(world);
    char *text = (char *)world;
    free(text);
}