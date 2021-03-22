#pragma once
#include "body.h"
#include "sprite.h"
#include "spritesheet.h"
#include "raylib.h"

typedef struct Particle {
    struct ParticleEffect *effect; // parent effect, 0 = dead
    struct Particle *next;         // when alive, next particle in effect. when dead, intrusive free list.
    Body3D body;                   // physics body
    Sprite sprite;                 // particle sprite
    Color color;                   // particle color (tint if particle also has sprite)
    double spawnAt;                // time to spawn (relative to effect->startedAt)
    double dieAt;                  // time to die   (relative to effect->startedAt)
} Particle;

void       particles_init   ();
void       particles_free   ();
size_t     particles_active ();
void       particles_update (double now, double dt);
void       particles_draw   (double now, double dt);

//-----------------------------------------------------------------------------

typedef enum ParticleEffectType {
    ParticleEffectType_Dead,
    ParticleEffectType_Blood,
    ParticleEffectType_Gold,
    ParticleEffectType_Goo,
    ParticleEffectType_Count
} ParticleEffectType;

typedef enum ParticleEffectEventType {
    ParticleEffectEvent_BeforeUpdate,
    ParticleEffectEvent_Dying,
    ParticleEffectEvent_Count
} ParticleEffectEventType;

typedef void (*ParticleInitFn  )(Particle *particle, double duration);
typedef void (*ParticleUpdateFn)(Particle *particle, float alpha);

typedef struct ParticleDef {
    ParticleInitFn   init;
    ParticleUpdateFn update;
} ParticleDef;

typedef void (*ParticeEffectEventCallbackFn)(struct ParticleEffect *effect, void *userData);

typedef struct ParticeEffectEventCallback {
    ParticeEffectEventCallbackFn function;
    void *userData;
} ParticeEffectEventCallback;

typedef struct ParticleEffect {
    ParticleEffectType type;      // type of particle effect (or Dead)
    size_t particlesLeft;         // number of particles that are pending or alive (i.e. not dead)
    Vector3 origin;               // origin of particle effect
    double duration;              // time to play effect for
    double startedAt;             // time started
    Sprite sprite;                // sprite to be used for all particles.. for now
    struct ParticleEffect *next;  // when dead, intrusive free list
    ParticleDef *def;
    ParticeEffectEventCallback callbacks[ParticleEffectEvent_Count];
} ParticleEffect;

ParticleEffect *particle_effect_create(ParticleEffectType type, size_t particleCount, Vector3 origin, double duration,
                                       double now, const SpriteDef *spriteDef);
size_t          particle_effects_active();
