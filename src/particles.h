#pragma once
#include "raylib.h"

typedef enum ParticleEffectType {
    ParticleEffect_Blood,
    ParticleEffect_Count
} ParticleEffectType;

typedef enum ParticleEffectState {
    ParticleState_Dead,
    ParticleState_Alive,
    //ParticleState_Paused,
} ParticleEffectState;

typedef struct Particle {
    double spawnAt;
    double dieAt;
    Vector2 acceleration;
    Vector2 velocity;
    Vector2 position;
    float rotation;
    float scale;
    Color color;
} Particle;

typedef struct ParticleEffect {
    //size_t animChannelCount;
    //AnimChannel *animChannels;
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
void particle_effect_start      (ParticleEffect *particleEffect, double time, Vector2 origin);
void particle_effect_stop       (ParticleEffect *particleEffect);
void particle_effect_update     (ParticleEffect *particleEffect, double time);
void particle_effect_draw       (ParticleEffect *particleEffect, double time);
void particle_effect_free       (ParticleEffect *particleEffect);