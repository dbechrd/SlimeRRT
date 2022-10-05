#include "shadow.h"
#include "raylib/raylib.h"

void Shadow::Draw(Vector3 pos, float radius, float yOffset, float alphaPercent)
{
    const float scale = MAX(0.1f, 1.0f - pos.z / METERS_TO_PIXELS(2));
    DrawEllipse((int)pos.x, (int)(pos.y + yOffset), radius * scale, radius * scale * 0.5f, Fade(BLACK, 0.5f * alphaPercent));
}