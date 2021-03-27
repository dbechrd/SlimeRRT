#include "helpers.h"
#include "raylib.h"

static Color ShadowColor(const Color color)
{
    Color shadow_color = color;
    shadow_color.r = (unsigned char)(color.r * 0.0f);
    shadow_color.g = (unsigned char)(color.g * 0.0f);
    shadow_color.b = (unsigned char)(color.b * 0.0f);
    return shadow_color;
}

void DrawTextFont(Font font, const char *text, float posX, float posY, int fontSize, Color color)
{
    // Check if default font has been loaded
    if (font.texture.id != 0)
    {
        Vector2 position = { posX, posY };
        Vector2 shadowPosition = position;
        shadowPosition.x++;
        shadowPosition.y++;

        int defaultFontSize = 10;   // Default Font chars height in pixel
        if (fontSize < defaultFontSize) fontSize = defaultFontSize;
        int spacing = fontSize/defaultFontSize;

        DrawTextEx(font, text, shadowPosition, (float)fontSize, (float)spacing, ShadowColor(color));
        DrawTextEx(font, text, position, (float)fontSize, (float)spacing, color);
    }
}

#define DLB_RAND_IMPLEMENTATION
#include "dlb_rand.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if 0
#pragma warning(push)
#pragma warning(disable: 4244)  // conversion from 'int' to 'float'
#pragma warning(disable: 4267)  // conversion from 'size_t' to 'int'
#define GUI_TEXTBOX_EXTENDED_IMPLEMENTATION
#include "gui_textbox_extended.h"
#pragma warning(pop)
#endif