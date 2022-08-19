#pragma once

namespace FX {
    void blood_init          (Particle &particle);
    void blood_update        (Particle &particle, float alpha);
    void copper_init         (Particle &particle);
    void copper_update       (Particle &particle, float alpha);
    void gem_init            (Particle &particle);
    void gem_update          (Particle &particle, float alpha);
    void golden_chest_init   (Particle &particle);
    void golden_chest_update (Particle &particle, float alpha);
    void goo_init            (Particle &particle);
    void goo_update          (Particle &particle, float alpha);
    void number_init         (Particle &particle);
    void number_update       (Particle &particle, float alpha);
    void rainbow_init        (Particle &particle);
    void rainbow_update      (Particle &particle, float alpha);
}