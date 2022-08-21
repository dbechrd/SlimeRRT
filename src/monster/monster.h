#pragma once

struct Enemy;

namespace Monster
{
    extern void slime_init(Enemy &enemy);
    extern void slime_update(Enemy &enemy, double dt, World &world);
}