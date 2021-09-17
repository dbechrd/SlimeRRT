#pragma once
#include "body.h"
#include "draw_command.h"
#include "drawable.h"
#include "sprite.h"
#include "spritesheet.h"
#include "raylib/raylib.h"

struct ParticleEffect;

struct Particle : public Drawable {
    ParticleEffect * effect  {};  // parent effect, 0 = dead
    Particle *       next    {};  // when alive, next particle in effect. when dead, intrusive free list.
    Body3D           body    {};  // physics body
    Color            color   {};  // particle color (tint if particle also has sprite)
    double           spawnAt {};  // time to spawn (relative to effect->startedAt)
    double           dieAt   {};  // time to die   (relative to effect->startedAt)

    float Depth() const override;
    bool Cull(const Rectangle &cullRect) const override;
    void Draw() const override;
};

void       particles_init   (void);
void       particles_free   (void);
size_t     particles_active (void);
void       particles_update (double now, double dt);
void       particles_push   (DrawList &drawList);

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

struct ParticeEffectEventCallback {
    ParticeEffectEventCallbackFn function {};
    void *                       userData {};
};

struct ParticleEffect {
    ParticleEffectType type          {};      // type of particle effect (or Dead)
    size_t             particlesLeft {};         // number of particles that are pending or alive (i.e. not dead)
    Vector3            origin        {};               // origin of particle effect
    double             duration      {};              // time to play effect for
    double             startedAt     {};             // time started
    Sprite             sprite        {};                // sprite to be used for all particles.. for now
    ParticleEffect *   next          {};  // when dead, intrusive free list
    ParticleDef *      def           {};
    ParticeEffectEventCallback callbacks[(int)ParticleEffectEventType::Count]{};
};

ParticleEffect *particle_effect_create  (ParticleEffectType type, size_t particleCount, Vector3 origin, double duration, double now, const SpriteDef *spriteDef);
size_t          particle_effects_active ();
