#pragma once

namespace FX {
    void blood_init          (Particle &particle, ParticleEffect &effect);
    void blood_update        (Particle &particle, float alpha);
    void gem_init            (Particle &particle, ParticleEffect &effect);
    void gem_update          (Particle &particle, float alpha);
    void gold_init           (Particle &particle, ParticleEffect &effect);
    void gold_update         (Particle &particle, float alpha);
    void golden_chest_init   (Particle &particle, ParticleEffect &effect);
    void golden_chest_update (Particle &particle, float alpha);
    void goo_init            (Particle &particle, ParticleEffect &effect);
    void goo_update          (Particle &particle, float alpha);
}