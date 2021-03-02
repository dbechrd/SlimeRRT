#pragma once
#include "raylib.h"
#include <stdbool.h>

typedef enum ParticleState {
    ParticleState_Dead  = 0,
    ParticleState_Alive = 1,
} ParticleState;

typedef struct Particle {
    ParticleState state;
    double spawnAt;
    double dieAt;
    Vector2 acceleration;
    Vector2 velocity;
    Vector2 position;
    float rotation;
    float scale;
    Color color;
} Particle;

typedef enum ParticleEffectType {
    ParticleEffectType_Blood,
    ParticleEffectType_Count
} ParticleEffectType;

typedef enum ParticleEffectState {
    ParticleEffectState_Dead  = 0,
    ParticleEffectState_Alive = 1,
    //ParticleState_Paused,
} ParticleEffectState;

typedef struct ParticleEffect {
    ParticleEffectType type;
    ParticleEffectState state;  // dead or alive (and maybe paused in the future?)
    Vector2 origin;             // origin of particle effect
    double startedAt;           // time started
    double lastUpdatedAt;       // time last updated
    double duration;            // time to play effect for
    size_t particleNext;        // next free particle (for effects that add new ones over time)
    size_t particleCount;       // number of particles in "particles"
    Particle *particles;        // particle data
} ParticleEffect;

void particle_effect_generate   (ParticleEffect *particleEffect, ParticleEffectType type, size_t particleCount, double duration);
bool particle_effect_start      (ParticleEffect *particleEffect, double time, Vector2 origin);
void particle_effect_stop       (ParticleEffect *particleEffect);
void particle_effect_update     (ParticleEffect *particleEffect, double time);
void particle_effect_draw       (ParticleEffect *particleEffect, double time);
void particle_effect_free       (ParticleEffect *particleEffect);