#pragma once
#include "body.h"
#include "draw_command.h"
#include "catalog/particle_fx.h"
#include "sprite.h"
#include "raylib/raylib.h"

struct ParticleEffect;

struct Particle : Drawable {
    ParticleEffect *effect  {};  // parent effect, 0 = dead
    Particle       *next    {};  // when alive, next particle in effect. when dead, intrusive free list.
    Body3D          body    {};  // physics body
    Sprite          sprite  {};
    const char      *text   {};  // text to render (instead of sprite)
    Color           color   {};  // particle color (tint if particle also has sprite)
    double          spawnAt {};  // time to spawn
    bool            alive   {};  // currently alive
    double          dieAt   {};  // time to die

    float Depth(void) const;
    bool  Cull(const Rectangle& cullRect) const;
    void  Draw(World &world);
};

//-----------------------------------------------------------------------------

enum class ParticleEffect_Event {
    BeforeUpdate,
    Dying,
    Count
};

enum class ParticleEffect_ParticleEvent {
    AfterUpdate,
    Draw,
    Died,
    Count
};

struct ParticleEffect_Callback {
    void (*callback)(struct ParticleEffect &effect, void *userData);
    void *userData{};
};

struct ParticleEffect_ParticleCallback {
    void (*callback)(struct Particle &particle, void *userData);
    void *userData{};
};

struct ParticleEffectParams {
    int particleCountMin  = 64;
    int particleCountMax  = 64;
    float durationMin     = 10.0f;
    float durationMax     = 10.0f;
    float spawnDelayMin   = 0.0f;
    float spawnDelayMax   = 5.0f;
    float lifespanMin     = 1.0f;
    float lifespanMax     = 5.0f;
    float spawnScaleFirst = 8.0f;  // scale that first particle spawns at
    float spawnScaleLast  = 1.0f;  // scale that last particle spawns at
    float velocityXMin    = -1.0f;
    float velocityXMax    = 1.0f;
    float velocityYMin    = -1.0f;
    float velocityYMax    = 1.0f;
    float velocityZMin    = 2.0f;
    float velocityZMax    = 2.0f;
    float friction        = 0.5f;
};

struct ParticleEffect {
    Catalog::ParticleEffectID id            {};  // itemClass of particle effect
    size_t                    particlesLeft {};  // number of particles that are pending or alive (i.e. not dead)
    Vector3                   origin        {};  // origin of particle effect
    double                    duration      {};  // time to play effect for
    double                    startedAt     {};  // time started
    Sprite                    sprite        {};  // sprite to be used for all particles.. for now
    ParticleEffectParams      params        {};  // parameters
    ParticleEffect            *next         {};  // when dead, intrusive free list

    ParticleEffect_Callback effectCallbacks[(size_t)ParticleEffect_Event::Count]{};
    ParticleEffect_ParticleCallback particleCallbacks[(size_t)ParticleEffect_ParticleEvent::Count]{};
};

//-----------------------------------------------------------------------------

#define MAX_EFFECTS   32
#define MAX_PARTICLES 1024

struct ParticleSystem {
    ParticleSystem  (void);
    ~ParticleSystem (void);

    size_t ParticlesActive (void);
    size_t EffectsActive   (void);

    Particle *ParticlePool(void);

    ParticleEffect *GenerateEffect(Catalog::ParticleEffectID type, Vector3 origin, const ParticleEffectParams &par);
    void Update  (double dt);
    void PushAll (DrawList &drawList);

private:
    Particle *Alloc (void);

    ParticleEffect  effects[MAX_EFFECTS] {};
    ParticleEffect *effectsFree          {};
    size_t          effectsActiveCount   {};

    Particle  particles[MAX_PARTICLES] {};
    Particle *particlesFree            {};
    size_t    particlesActiveCount     {};
};

//-----------------------------------------------------------------------------

void ParticlesFollowPlayerGut(ParticleEffect &effect, void *userData);
void ParticleDrawText(Particle &particle, void *userData);
void ParticleFreeText(ParticleEffect &effect, void *userData);
