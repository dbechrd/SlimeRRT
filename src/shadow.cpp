#include "shadow.h"
#include "raylib/raylib.h"

void Shadow::Draw(int x, int y, int radius, int yOffset)
{
    DrawEllipse(x, y + (int)yOffset, (float)radius, radius / 2.0f, Fade(BLACK, 0.5f));
}