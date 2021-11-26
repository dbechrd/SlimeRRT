#pragma once
#include "helpers.h"

enum class SoundID {
    Empty,
    Footstep,
    Gold,
    Slime_Stab1,
    Squeak,
    Squish1,
    Squish2,
    Whoosh,
    GemBounce,
    Count
};

void sound_catalog_init    ();
void sound_catalog_play    (SoundID id, float pitch);
bool sound_catalog_playing (SoundID id);
void sound_catalog_free    ();