#include "maths.h"

bool CheckCollisionCircleRecti(const Vector2i &center, int radius, const Recti &rec)
{
    int recCenterX = (int)(rec.x + rec.width / 2);
    int recCenterY = (int)(rec.y + rec.height / 2);

    int dx = abs(center.x - recCenterX);
    int dy = abs(center.y - recCenterY);

    if (dx > (rec.width / 2 + radius)) { return false; }
    if (dy > (rec.height / 2 + radius)) { return false; }

    if (dx <= (rec.width / 2)) { return true; }
    if (dy <= (rec.height / 2)) { return true; }

    int cornerDistanceSq =
        (dx - rec.width / 2) * (dx - rec.width / 2) +
        (dy - rec.height / 2) * (dy - rec.height / 2);

    const bool collision = (cornerDistanceSq <= (radius * radius));
    return collision;
}