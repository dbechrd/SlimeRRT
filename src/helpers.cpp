#include "helpers.h"
#include "raylib/raylib.h"
#include <cassert>
#include <cmath>
#include <cstdio>

Shader g_sdfShader{};

static Color ShadowColor(const Color color)
{
    Color shadow_color = color;
    shadow_color.r = (unsigned char)(color.r * 0.0f);
    shadow_color.g = (unsigned char)(color.g * 0.0f);
    shadow_color.b = (unsigned char)(color.b * 0.0f);
    return shadow_color;
}

void DrawTextFont(Font font, const char *text, float posX, float posY, float offsetX, float offsetY, int fontSize, const Color &color)
{
    if (!text) {
        return;
    }

    int shadowOffset = 2;
    if (fontSize >= 54) {
        shadowOffset = 3;
    } else if (fontSize >= 24) {
        shadowOffset = 2;
    }
    // Check if default font has been loaded
    if (font.texture.id != 0) {
        Vector2 position = { posX, posY };
        Vector2 shadowPosition = position;
        shadowPosition.x += shadowOffset;
        shadowPosition.y += shadowOffset;

        int defaultFontSize = 10;   // Default Font chars height in pixel
        if (fontSize < defaultFontSize) fontSize = defaultFontSize;
        int spacing = fontSize / defaultFontSize;

        // Assume SDF font if grayscale
        const bool sdfFont = font.glyphs->image.format == PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
        if (sdfFont) BeginShaderMode(g_sdfShader);
        DrawTextEx(font, text, { (float)shadowPosition.x - offsetX, (float)shadowPosition.y - offsetY }, (float)fontSize, (float)spacing, ShadowColor(color));
        DrawTextEx(font, text, position, (float)fontSize, (float)spacing, color);
        if (sdfFont) EndShaderMode();
    }
}

// NOTE(dlb): Thread-local version of Raylib's TextFormat to prevent races causing reads from the wrong index
// WARNING: String returned will expire after this function is called MAX_TEXTFORMAT_BUFFERS times
const char *SafeTextFormat(const char *text, ...)
{
#ifndef MAX_TEXTFORMAT_BUFFERS
#define MAX_TEXTFORMAT_BUFFERS 4        // Maximum number of static buffers for text formatting
#endif

#ifndef MAX_TEXT_BUFFER_LENGTH
#define MAX_TEXT_BUFFER_LENGTH 1024     // Maximum length of each buffer
#endif

    // We create an array of buffers so strings don't expire until MAX_TEXTFORMAT_BUFFERS invocations
    thread_local char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = { 0 };
    thread_local int index = 0;

    char *currentBuffer = buffers[index];
    memset(currentBuffer, 0, MAX_TEXT_BUFFER_LENGTH);

    va_list args;
    va_start(args, text);
    vsnprintf(currentBuffer, MAX_TEXT_BUFFER_LENGTH, text, args);
    va_end(args);

    index += 1; // Move to next buffer for next function call
    if (index >= MAX_TEXTFORMAT_BUFFERS) index = 0;

    return currentBuffer;
}

const char *SafeTextFormatIP(ENetAddress &address)
{
    //char asStr[64]{};
    //enet_address_get_host_ip_new(&address, asStr, sizeof(asStr) - 1);
    //const char *text = SafeTextFormat("%s:%hu", asStr, address.port);

    // TODO(dlb)[cleanup]: Manual extraction of IPv4 address (was used by zed_net)
    //unsigned char bytes[4] = {};
    //bytes[0] = address.host & 0xFF;
    //bytes[1] = (address.host >> 8) & 0xFF;
    //bytes[2] = (address.host >> 16) & 0xFF;
    //bytes[3] = (address.host >> 24) & 0xFF;
    //const char *text = SafeTextFormat("%d.%d.%d.%d:%hu", bytes[0], bytes[1], bytes[2], bytes[3], address.port);

    const char *text = SafeTextFormat("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
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

const char *SafeTextFormatTimestamp()
{
    thread_local char timestampStr[TIMESTAMP_LENGTH];
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

bool PointInRect(Vector2 &point, Rectangle &rect)
{
    return (point.x >= rect.x && point.y >= rect.y && point.x < rect.x + rect.width && point.y < rect.y + rect.height);
}