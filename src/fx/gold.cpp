#include "particles.h"
#include "dlb_rand.h"
#include <cassert>

namespace FX {
    void gold_init(Particle &particle, ParticleEffect &effect)
    {
        // Spawn randomly during first 5% of duration
        particle.spawnAt = effect.duration * dlb_rand32f_variance(0.05f);

        // Die randomly during last 15% of animation
        particle.dieAt = effect.duration - (effect.duration * dlb_rand32f_variance(0.15f));
        assert(particle.dieAt > particle.spawnAt);

    #if 1
        Vector3i randVel {};
        randVel.x = dlb_rand32i_variance(METERS_TO_UNITS(1));
        randVel.y = dlb_rand32i_variance(METERS_TO_UNITS(1));
        randVel.z = dlb_rand32i_range(0, METERS_TO_UNITS(4));
        particle.body.velocity = randVel;
        particle.body.restitution = 0.8f;
        particle.body.friction = 0.5f;
    #else
        const float direction = 1.0f;
        float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
        particle.velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
    #endif
        //particle.position = (Vector2){ 0.0f, 0.0f };
        particle.color = WHITE;
        particle.sprite.scale = 1;

        static const SpriteDef *coinSpriteDef{};
        if (!coinSpriteDef) {
            const Spritesheet &coinSpritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Coin);
            coinSpriteDef = coinSpritesheet.FindSprite("coin");

            // TODO: Don't play particle effects on the server so that we can re-enable this assert on client side
            //assert(coinSpriteDef);
        }
        particle.sprite.spriteDef = coinSpriteDef;
    }

    void gold_update(Particle &particle, float alpha)
    {
        UNUSED(particle);
        UNUSED(alpha);
    }
}