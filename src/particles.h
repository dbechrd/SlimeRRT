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
    Color           color   {};  // particle color (tint if particle also has sprite)
    double          spawnAt {};  // time to spawn (relative to effect->startedAt)
    double          dieAt   {};  // time to die   (relative to effect->startedAt)

    float Depth(void) const;
    bool  Cull(const Rectangle& cullRect) const;
    void  Draw(void) const;
};

//-----------------------------------------------------------------------------

enum class ParticleEffect_Event {
    BeforeUpdate,
    Dying,
    Count
};

enum class ParticleEffect_ParticleEvent {
    AfterUpdate,
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

struct ParticleEffect {
    Catalog::ParticleEffectID id   {};  // type of particle effect
    size_t           particlesLeft {};  // number of particles that are pending or alive (i.e. not dead)
    Vector3          origin        {};  // origin of particle effect
    double           duration      {};  // time to play effect for
    double           startedAt     {};  // time started
    Sprite           sprite        {};  // sprite to be used for all particles.. for now
    ParticleEffect  *next          {};  // when dead, intrusive free list

    ParticleEffect_Callback effectCallbacks[(size_t)ParticleEffect_Event::Count]{};
    ParticleEffect_ParticleCallback particleCallbacks[(size_t)ParticleEffect_Event::Count]{};
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

    ParticleEffect *GenerateEffect(Catalog::ParticleEffectID id, size_t particleCount, Vector3 origin, double duration);
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