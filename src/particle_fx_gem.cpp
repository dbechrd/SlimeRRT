#include "particles.h"
#include "particle_fx.h"
#include "dlb_rand.h"
#include <cassert>

void particle_fx_gem_init(Particle &particle, ParticleFX &effect)
{
    // Spawn randomly during first 5% of duration
    particle.spawnAt = 0.75f + effect.duration * dlb_rand32f_variance(0.05f);

    // Die randomly during last 15% of animation
    particle.dieAt = effect.duration - (effect.duration * dlb_rand32f_variance(0.15f));
    assert(particle.dieAt > particle.spawnAt);

#if 1
    float randX = dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
    float randY = dlb_rand32f_variance(METERS_TO_PIXELS(1.0f));
    float randZ = dlb_rand32f_range(METERS_TO_PIXELS(3.0f), METERS_TO_PIXELS(6.0f));
    particle.body.velocity = { randX, randY, randZ };
    particle.body.restitution = 0.9f;
    particle.body.friction = 0.4f;
#else
    const float direction = 1.0f;
    float randX = direction * dlb_rand32f(0.0f, METERS_TO_PIXELS(1.0f));
    particle.velocity = (Vector2){ randX, METERS_TO_PIXELS(-3.0f) };
#endif
    //particle.position = (Vector2){ 0.0f, 0.0f };
    particle.color = WHITE;
    particle.sprite.scale = 1.0f;

    const Spritesheet &itemSpritesheet = SpritesheetCatalog::spritesheets[(int)SpritesheetID::Items];
    uint32_t randomGemIdx = dlb_rand32u_range(0, (uint32_t)itemSpritesheet.sprites.size() - 2);
    particle.sprite.spriteDef = &itemSpritesheet.sprites[randomGemIdx];

    // TODO: Don't play particle effects on the server so that we can re-enable this assert on client side
    //assert(coinSpriteDef);
}

void particle_fx_gem_update(Particle &particle, float alpha)
{
    UNUSED(particle);
    UNUSED(alpha);

    if (particle.body.bounced) {
        sound_catalog_play(SoundID::GemBounce, 1.0f + dlb_rand32f_variance(0.3f));
    }
}
