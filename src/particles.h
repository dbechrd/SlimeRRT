#pragma once
#include "body.h"
#include "draw_command.h"
#include "sprite.h"
#include "spritesheet.h"
#include "raylib/raylib.h"

struct ParticleFX;

struct Particle {
    ParticleFX *effect  {};  // parent effect, 0 = dead
    Particle   *next    {};  // when alive, next particle in effect. when dead, intrusive free list.
    Body3D      body    {};  // physics body
    Sprite      sprite  {};
    Color       color   {};  // particle color (tint if particle also has sprite)
    double      spawnAt {};  // time to spawn (relative to effect->startedAt)
    double      dieAt   {};  // time to die   (relative to effect->startedAt)
};

float particle_depth (const Drawable &drawable);
bool  particle_cull  (const Drawable &drawable, const Rectangle &cullRect);
void  particle_draw  (const Drawable &drawable);

//-----------------------------------------------------------------------------

enum ParticleFX_Type {
    ParticleFX_Dead,
    ParticleFX_Blood,
    ParticleFX_Gold,
    ParticleFX_Goo,
    ParticleFX_Count
};

enum ParticleFxEvent_Type {
    ParticleFXEvent_BeforeUpdate,
    ParticleFXEvent_Dying,
    ParticleFXEvent_Count
};

typedef void (*ParticeFX_FnEventCallback)(struct ParticleFX &effect, void *userData);

struct ParticleFXEvent_Callback {
    ParticeFX_FnEventCallback function {};
    void *                    userData {};
};

struct ParticleFX {
    ParticleFX_Type type          {};  // type of particle effect (or Dead)
    size_t          particlesLeft {};  // number of particles that are pending or alive (i.e. not dead)
    Vector3         origin        {};  // origin of particle effect
    double          duration      {};  // time to play effect for
    double          startedAt     {};  // time started
    Sprite          sprite        {};  // sprite to be used for all particles.. for now
    ParticleFX *    next          {};  // when dead, intrusive free list
    ParticleFXEvent_Callback callbacks[ParticleFX_Count] {};
};

//-----------------------------------------------------------------------------

#define MAX_EFFECTS   32
#define MAX_PARTICLES 1024

typedef void (*ParticleFX_FnInit  )(Particle &particle, ParticleFX &effect);
typedef void (*ParticleFX_FnUpdate)(Particle &particle, float alpha);

struct ParticleFXDef {
    ParticleFX_FnInit   init   {};
    ParticleFX_FnUpdate update {};
};

struct ParticleSystem {
    ParticleSystem  (void);
    ~ParticleSystem (void);

    size_t ParticlesActive (void);
    size_t EffectsActive   (void);

    ParticleFX *GenerateFX (ParticleFX_Type type, size_t particleCount, Vector3 origin, double duration, double now);
    void        Update     (double now, double dt);
    void        Push       (DrawList &drawList);

private:
    Particle *Alloc (void);
    void      Push  (DrawList &drawList, const Particle &particle);

    ParticleFX  effects[MAX_EFFECTS] {};
    ParticleFX *effectsFree          {};
    size_t      effectsActiveCount   {};

    Particle  particles[MAX_PARTICLES] {};
    Particle *particlesFree            {};
    size_t    particlesActiveCount     {};

    ParticleFXDef registry[ParticleFX_Count] {};
};

