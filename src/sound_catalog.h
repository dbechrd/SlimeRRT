#pragma once
#include "helpers.h"

enum SoundID {
    SoundID_Empty,
    SoundID_Footstep,
    SoundID_Gold,
    SoundID_Slime_Stab1,
    SoundID_Squeak,
    SoundID_Squish1,
    SoundID_Squish2,
    SoundID_Whoosh,
    SoundID_Count
};

void sound_catalog_init();
void sound_catalog_play(SoundID id, float pitch);
bool sound_catalog_playing(SoundID id);
void sound_catalog_free();