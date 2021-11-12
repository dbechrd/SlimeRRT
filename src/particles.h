#pragma once
#include "body.h"
#include "draw_command.h"
#include "sprite.h"
#include "spritesheet.h"
#include "raylib/raylib.h"

struct ParticleEffect;

struct Particle {
    ParticleEffect * effect  {};  // parent effect, 0 = dead
    Particle *       next    {};  // when alive, next particle in effect. when dead, intrusive free list.
    Body3D           body    {};  // physics body
    Sprite           sprite  {};
    Color            color   {};  // particle color (tint if particle also has sprite)
    double           spawnAt {};  // time to spawn (relative to effect->startedAt)
    double           dieAt   {};  // time to die   (relative to effect->startedAt)
};

float  particle_depth (const Drawable &drawable);
bool   particle_cull  (const Drawable &drawable, const Rectangle &cullRect);
void   particle_draw  (const Drawable &drawable);

//-----------------------------------------------------------------------------

enum class ParticleEffectType {
    Dead,
    Blood,
    Gold,
    Goo,
    Count
};

enum class ParticleEffectEventType {
    BeforeUpdate,
    Dying,
    Count
};

typedef void (*ParticleInitFn  )(Particle &particle, double duration);
typedef void (*ParticleUpdateFn)(Particle &particle, float alpha);

struct ParticleDef {
    ParticleInitFn   init   {};
    ParticleUpdateFn update {};
};

typedef void (*ParticeEffectEventCallbackFn)(struct ParticleEffect &effect, void *userData);

struct ParticleEffectEventCallback {
    ParticeEffectEventCallbackFn function {};
    void *                       userData {};
};

struct ParticleEffect {
    ParticleEffectType type          {};  // type of particle effect (or Dead)
    size_t             particlesLeft {};  // number of particles that are pending or alive (i.e. not dead)
    Vector3            origin        {};  // origin of particle effect
    double             duration      {};  // time to play effect for
    double             startedAt     {};  // time started
    Sprite             sprite        {};  // sprite to be used for all particles.. for now
    ParticleEffect *   next          {};  // when dead, intrusive free list
    ParticleDef *      def           {};
    ParticleEffectEventCallback callbacks[(int)ParticleEffectEventType::Count]{};
};

//-----------------------------------------------------------------------------

#define MAX_EFFECTS   32
#define MAX_PARTICLES 1024

struct ParticleSystem {
    ParticleSystem  (void);
    ~ParticleSystem (void);
    
    size_t ParticlesActive(void);
    size_t EffectsActive(void);

    ParticleEffect *GenerateEffect(ParticleEffectType type, size_t particleCount, Vector3 origin, double duration, double now, const SpriteDef *spriteDef);
    void            Update        (double now, double dt);
    void            PushParticles (DrawList &drawList);

private:
    Particle *Alloc(void);
    void GenerateEffectParticles(ParticleEffect *effect, size_t particleCount, const SpriteDef *spriteDef);
    void Push(DrawList &drawList, const Particle &particle);

    ParticleEffect effects[MAX_EFFECTS] {};
    ParticleEffect *effectsFree         {};
    size_t effectsActiveCount           {};

    Particle particles[MAX_PARTICLES]   {};
    Particle *particlesFree             {};
    size_t particlesActiveCount         {};
};

