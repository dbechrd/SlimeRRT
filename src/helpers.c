#define DLB_RAND_IMPLEMENTATION
#include "dlb_rand.h"

#include "raylib.h"

void DrawTextFont(Font font, const char *text, int posX, int posY, int fontSize, Color color)
{
    // Check if default font has been loaded
    if (font.texture.id != 0)
    {
        Vector2 position = { (float)posX, (float)posY };

        int defaultFontSize = 10;   // Default Font chars height in pixel
        if (fontSize < defaultFontSize) fontSize = defaultFontSize;
        int spacing = fontSize/defaultFontSize;

        DrawTextEx(font, text, position, (float)fontSize, (float)spacing, color);
    }
}