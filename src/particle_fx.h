#pragma once

typedef struct Particle Particle;

#define PARTICLE_FX_INIT(name) \
    void particle_fx_##name##_init   (Particle *particle, double duration)

#define PARTICLE_FX_UPDATE(name) \
    void particle_fx_##name##_update (Particle *particle, float alpha)

#define PARTICLE_FX_DECL(name) \
    PARTICLE_FX_INIT(name); \
    PARTICLE_FX_UPDATE(name); \

PARTICLE_FX_DECL(blood);
PARTICLE_FX_DECL(gold);
PARTICLE_FX_DECL(goo);

#undef PARTICLE_FX_DECL