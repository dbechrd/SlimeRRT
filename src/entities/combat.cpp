#include "combat.h"

float Combat::TakeDamage(float damage)
{
    DLB_ASSERT(damage > 0);
    if (diedAt || flags & Flag_TooBigToFail) {
        return 0;
    }
    DLB_ASSERT(hitPoints);
    DLB_ASSERT(hitPointsMax);

    hitPointsPrev = hitPoints;
    const float dealt = CLAMP(damage, 0.0f, hitPoints);
    hitPoints -= dealt;
    if (!hitPoints) {
        diedAt = g_clock.now;
    }
    return dealt;
}

// Returns number of levels the entity gained due to the xp (or 0)
uint8_t Combat::GrantXP(uint32_t xpToGrant)
{
    uint8_t levelsGained = 0;
    xp += xpToGrant;
    int overflowXp = xp - (level * 20u);
    while (overflowXp >= 0 && level < UINT8_MAX) {
        level++;
        xp = (uint32_t)overflowXp;
        levelsGained++;
    }
    return levelsGained;
}

void Combat::Update(double dt)
{
    DLB_ASSERT(hitPointsMax);
    hitPointsSmooth += (hitPoints - hitPointsSmooth) * CLAMP(5.0f * (float)dt, 0.05f, 1.0f);
}