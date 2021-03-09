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
    ParticleEffectState_Count
} ParticleEffectState;

typedef enum ParticleEffectEventType {
    ParticleEffectEvent_Starting,
    ParticleEffectEvent_Stopping,
    ParticleEffectEvent_Dying,
    ParticleEffectEvent_Count
} ParticleEffectEventType;

typedef void (*ParticeEffectEventCallback)(struct ParticleEffect *particleEffect);

typedef struct ParticleEffect {
    ParticleEffectType type;
    ParticleEffectState state;  // dead or alive (and maybe paused in the future?)
    Vector3 origin;             // origin of particle effect
    double startedAt;           // time started
    double lastUpdatedAt;       // time last updated
    double duration;            // time to play effect for
    size_t particleNext;        // next free particle (for effects that add new ones over time)
    size_t particleCount;       // number of particles in "particles"
    Particle *particles;        // particle data
    ParticeEffectEventCallback callbacks[ParticleEffectEvent_Count];
} ParticleEffect;

void particle_effect_generate   (ParticleEffect *particleEffect, ParticleEffectType type, size_t particleCount, double duration);
bool particle_effect_start      (ParticleEffect *particleEffect, double time, Vector3 origin);
void particle_effect_stop       (ParticleEffect *particleEffect);
void particle_effect_update     (ParticleEffect *particleEffect, double time);
void particle_effect_draw       (ParticleEffect *particleEffect, double time);
void particle_effect_free       (ParticleEffect *particleEffect);