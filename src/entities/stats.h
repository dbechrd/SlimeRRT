#pragma once
#include "facet.h"
#include "entity.h"

// TODO: Put this somewhere else
// TODO: Silly idea: Make everything a type/count pair that goes
// into some invisible inventory slot. Then just serialize the
// relevant "items" in an NPC's "inventory" over the network to
// replicate the whole thing. Inventory = components!?
struct Stats : public Facet {
    float    damageDealt{};
    float    kmWalked{};
    uint32_t entitiesSlain[Entity_Count]{};
    uint32_t timesFistSwung{};
    uint32_t timesSwordSwung{};
    //uint32_t coinsCollected  {};  // TODO: Money earned via selling? Or something else cool
};