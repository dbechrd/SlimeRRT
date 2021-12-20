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

const char *TextFormatIP(ENetAddress &address)
{
    //char asStr[64]{};
    //enet_address_get_host_ip_new(&address, asStr, sizeof(asStr) - 1);
    //const char *text = TextFormat("%s:%hu", asStr, address.port);

    // TODO(dlb)[cleanup]: Manual extraction of IPv4 address (was used by zed_net)
    //unsigned char bytes[4] = {};
    //bytes[0] = address.host & 0xFF;
    //bytes[1] = (address.host >> 8) & 0xFF;
    //bytes[2] = (address.host >> 16) & 0xFF;
    //bytes[3] = (address.host >> 24) & 0xFF;
    //const char *text = TextFormat("%d.%d.%d.%d:%hu", bytes[0], bytes[1], bytes[2], bytes[3], address.port);

    const char *text = TextFormat("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
        (int)address.host.u.Byte[0],  (int)address.host.u.Byte[1],
        (int)address.host.u.Byte[2],  (int)address.host.u.Byte[3],
        (int)address.host.u.Byte[4],  (int)address.host.u.Byte[5],
        (int)address.host.u.Byte[6],  (int)address.host.u.Byte[7],
        (int)address.host.u.Byte[8],  (int)address.host.u.Byte[9],
        (int)address.host.u.Byte[10], (int)address.host.u.Byte[11],
        (int)address.host.u.Byte[12], (int)address.host.u.Byte[13],
        (int)address.host.u.Byte[14], (int)address.host.u.Byte[15]);

    return text;
}

const char *TextFormatTimestamp()
{
    static char timestampStr[TIMESTAMP_LENGTH];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int len = snprintf(timestampStr, sizeof(timestampStr), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    assert(len < sizeof(timestampStr));
    return timestampStr;
}

float VolumeCorrection(float volume)
{
    // Exponential equation for approximate linear loudness, with direct linear fallback in the lower range
    float mapped = volume < 0.296f ? 0.1f * volume : exp(5.0f * volume - 5.0f);
    float clamped = CLAMP(mapped, 0.0f, 1.0f);
    return clamped;
}

int ZoomMipLevel(float invZoom)
{
    // TODO: Calculate this based on how many tiles will appear on the screen, rather than camera zoom
    // Alternatively, we could group nearby tiles of the same type together into large quads?
    const int zoomMipLevel = MAX(1, (int)invZoom / 8);
    return zoomMipLevel;
}