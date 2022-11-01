#pragma once

struct Particle;

namespace FX {
    extern void blood_init          (Particle &particle);
    extern void blood_update        (Particle &particle, float alpha);
    extern void copper_init         (Particle &particle);
    extern void copper_update       (Particle &particle, float alpha);
    extern void gem_init            (Particle &particle);
    extern void gem_update          (Particle &particle, float alpha);
    extern void golden_chest_init   (Particle &particle);
    extern void golden_chest_update (Particle &particle, float alpha);
    extern void goo_init            (Particle &particle);
    extern void goo_update          (Particle &particle, float alpha);
    extern void number_init         (Particle &particle);
    extern void number_update       (Particle &particle, float alpha);
    extern void rainbow_init        (Particle &particle);
    extern void rainbow_update      (Particle &particle, float alpha);
    extern void poison_nova_init    (Particle &particle);
    extern void poison_nova_update  (Particle &particle, float alpha);
}