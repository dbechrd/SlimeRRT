#include "helpers.h"
#include "raylib/raylib.h"

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

const char *TextFormatIP(ENetAddress address)
{
    char asStr[64]{};
    enet_address_get_host_ip_new(&address, asStr, sizeof(asStr) - 1);
    const char *text = TextFormat("%s:%hu", asStr, address.port);
    // TODO(dlb)[cleanup]: Manual extraction of IPv4 address (was used by zed_net)
    //unsigned char bytes[4] = {};
    //bytes[0] = address.host & 0xFF;
    //bytes[1] = (address.host >> 8) & 0xFF;
    //bytes[2] = (address.host >> 16) & 0xFF;
    //bytes[3] = (address.host >> 24) & 0xFF;
    //const char *text = TextFormat("%d.%d.%d.%d:%hu", bytes[0], bytes[1], bytes[2], bytes[3], address.port);
    return text;
}