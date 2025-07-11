#include "../particles.h"
#include "dlb_rand.h"

namespace FX {
    void golden_chest_init(Particle &particle)
    {
        DLB_ASSERT(particle.effect);
        const ParticleEffect &effect = *particle.effect;

        // Spawn randomly during first 5% of duration
        particle.spawnAt = effect.startedAt + effect.duration * (double)dlb_rand32f_variance(0.05f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.startedAt + effect.duration - (effect.duration * (double)dlb_rand32f_variance(0.15f));
        DLB_ASSERT(particle.dieAt > particle.spawnAt);

    #if 1
        particle.body.Teleport({ 0, 0, METERS_TO_PIXELS(1.0f) });
        particle.body.restitution = 0.2f;
        particle.body.friction = 0.1f;
    #else
        const float direction = 1.0f;
        float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
        particle.velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
    #endif
        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = WHITE;
        particle.sprite.scale = 2.0f;

        //const Spritesheet &itemSpritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Items);
        //particle.sprite.spriteDef = itemSpritesheet.FindSprite("item_golden_chest");

        // TODO: Don't play particle effects on the server so that we can re-enable this assert on client side
        //DLB_ASSERT(coinSpriteDef);
    }

    void golden_chest_update(Particle &particle, float alpha)
    {
        UNUSED(particle);
        UNUSED(alpha);
        DLB_ASSERT(false);
    }
}