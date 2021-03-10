#pragma once
#include "raylib.h"
#include "body.h"
#include <stdbool.h>

typedef enum ParticleState {
    ParticleState_Dead,
    ParticleState_Alive,
    ParticleState_Count
} ParticleState;

typedef struct Particle {
    ParticleState state;
    Body3D body;
    double spawnAt;
    double dieAt;
    float rotation;
    float scale;
    Color color;
} Particle;

typedef enum ParticleEffectType {
    ParticleEffectType_Blood,
    ParticleEffectType_Gold,
    ParticleEffectType_Count
} ParticleEffectType;

typedef enum ParticleEffectState {
    ParticleEffectState_Dead,
    ParticleEffectState_Alive,
    ParticleEffectState_Stopping,
    ParticleEffectState_Dying,
    ParticleEffectState_Count
} ParticleEffectState;

typedef enum ParticleEffectEventType {
    ParticleEffectEvent_Started,
    ParticleEffectEvent_BeforeUpdate,
    ParticleEffectEvent_Stopped,
    ParticleEffectEvent_Dying,
    ParticleEffectEvent_Count
} ParticleEffectEventType;

typedef void (*ParticeEffectEventCallbackFn)(struct ParticleEffect *effect, void *userData);

typedef struct ParticeEffectEventCallback {
    ParticeEffectEventCallbackFn function;
    void *userData;
} ParticeEffectEventCallback;

typedef struct ParticleEffect {
    ParticleEffectType type;        // type of particle effect
    ParticleEffectState state;      // dead or alive (and maybe paused in the future?)
    Vector3 origin;                 // origin of particle effect
    double startedAt;               // time started
    double lastUpdatedAt;           // time last updated
    double duration;                // time to play effect for
    size_t particleCount;           // number of particles in particle buffer
    Particle *particles;            // particle buffer
    ParticeEffectEventCallback callbacks[ParticleEffectEvent_Count];
} ParticleEffect;

ParticleEffect *particle_effect_alloc   (ParticleEffectType type, size_t particleCount);
bool particle_effect_start              (ParticleEffect *effect, double time, double duration, Vector3 origin);
void particle_effect_stop               (ParticleEffect *effect);
void particle_effects_update            (double time);
void particle_effects_draw              (double time);
void particle_effect_free               (ParticleEffect *effect);
void particle_effects_free              ();