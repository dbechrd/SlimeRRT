#include "catalog/particle_fx.h"
#include "fx/fx.h"

namespace Catalog {
    void ParticleFX::Load(void)
    {
        byId[(size_t)ParticleEffectID::Blood      ] = { FX::blood_init,        FX::blood_update        };
        byId[(size_t)ParticleEffectID::Gem        ] = { FX::gem_init,          FX::gem_update          };
        byId[(size_t)ParticleEffectID::Gold       ] = { FX::gold_init,         FX::gold_update         };
        byId[(size_t)ParticleEffectID::GoldenChest] = { FX::golden_chest_init, FX::golden_chest_update };
        byId[(size_t)ParticleEffectID::Goo        ] = { FX::goo_init,          FX::goo_update          };
    }

    const ParticleEffectDef &ParticleFX::FindById(ParticleEffectID id) const
    {
        return byId[(size_t)id];
    }
}