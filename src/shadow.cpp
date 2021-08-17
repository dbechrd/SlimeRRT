#include "shadow.h"
#include "raylib.h"

void Shadow::Draw(int x, int y, float radius, float yOffset)
{
    DrawEllipse(x, y + (int)yOffset, radius, radius / 2.0f, Fade(BLACK, 0.5f));
}