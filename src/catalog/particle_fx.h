#pragma once

struct Particle;
struct ParticleEffect;

namespace Catalog {
    enum class ParticleEffectID {
        Empty,
        Blood,
        Gem,
        Gold,
        GoldenChest,
        Goo,
        Count
    };

    typedef void (*ParticleEffect_FnInit)(Particle &particle, ParticleEffect &effect);
    typedef void (*ParticleEffect_FnUpdate)(Particle &particle, float alpha);

    struct ParticleEffectDef {
        ParticleEffect_FnInit   init   {};
        ParticleEffect_FnUpdate update {};
    };

    struct ParticleFX {
        void Load(void);
        const ParticleEffectDef &FindById(ParticleEffectID id) const;

    private:
        ParticleEffectDef byId[(size_t)ParticleEffectID::Count]{};
    } g_particleFx;
}