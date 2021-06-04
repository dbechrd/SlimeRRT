#pragma once

struct Combat {
    float maxHitPoints;
    float hitPoints;
    float meleeDamage;  // TODO: add min/max and randomly choose?
    double attackStartedAt;
    double attackDuration;
};