#pragma once

// TODO: Min damage, max damage, effects, etc.
typedef struct Weapon {
    float damage;
} Weapon;

typedef struct Combat {
    double attackStartedAt;
    double attackDuration;
    float maxHitPoints;
    float hitPoints;
    Weapon *weapon;
} Combat;