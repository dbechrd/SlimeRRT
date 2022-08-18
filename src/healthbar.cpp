#include "healthbar.h"
#include <cmath>
#include <cassert>

Font HealthBar::s_font;

void HealthBar::SetFont(const Font font)
{
    s_font = font;
}

void HealthBar::Draw(const Vector2 &topCenter, const char *name, const Combat &combat)
{
    const int fontSize = s_font.baseSize;
    assert(fontSize);

    Vector2 pad{ 4.0f, 2.0f };
    float x = topCenter.x;
    float y = topCenter.y - 10.0f;

    //const char *hpText = SafeTextFormat("HP: %.02f / %.02f", hitPoints, hitPointsMax);
    const char *hpText = 0;
    if (combat.diedAt) {
        assert(!combat.hitPoints);
        hpText = SafeTextFormat("Respawning in %0.1f ...", SV_RESPAWN_TIMER - (g_clock.now - combat.diedAt));
    } else if (combat.hitPointsMax) {
        hpText = SafeTextFormat("HP: %.f / %.f", combat.hitPoints, combat.hitPointsMax);
    }
#if CL_DEBUG_SHOW_LEVELS
    const char *nameText = name ? SafeTextFormat("%s (%u)", name, combat.level) : nullptr;
#else
    const char *nameText = name ? SafeTextFormat(name) : nullptr;
#endif

#if 1
    if (combat.hitPointsMax) {
        // HACK: Just show name and bar, no HP text.
        hpText = nameText;
        nameText = nullptr;
    }
#endif

    Rectangle hpRect{};
    Vector2 hpRectMeasure = MeasureTextEx(s_font, hpText, (float)fontSize, 1.0f);
    hpRect.width = hpRectMeasure.x;
    hpRect.height = hpRectMeasure.y;
    hpRect.x = x - hpRect.width / 2.0f;
    hpRect.y = y - fontSize;

    Rectangle nameRect{};
    if (nameText) {
        Vector2 nameRectMeasure = MeasureTextEx(s_font, nameText, (float)fontSize, 1.0f);
        nameRect.width = nameRectMeasure.x;
        nameRect.height = nameRectMeasure.y;
        nameRect.x = x - nameRect.width / 2.0f;
        nameRect.y = y - fontSize;
        if (combat.hitPointsMax) {
            nameRect.y -= 4.0f + fontSize;
        }
    } else {
        nameRect.x = FLT_MAX;
        nameRect.y = FLT_MAX;
    }

    Rectangle bgRect{};
    bgRect.x = MIN(hpRect.x, nameRect.x) - pad.x;
    bgRect.y = hpRect.y - pad.y;
    bgRect.width  = MAX(hpRect.width, nameRect.width) + pad.x * 2.0f;
    bgRect.height = hpRect.height + pad.y * 2.0f;

    // Draw background
    DrawRectangleRec(bgRect, Fade(BLACK, 0.5f));

    if (combat.hitPointsMax) {
        // Draw animated white delta indicator
        Rectangle deltaIndicatorRect = bgRect;
        deltaIndicatorRect.width *= combat.hitPointsSmooth / combat.hitPointsMax;
        DrawRectangleRec(deltaIndicatorRect, WHITE);

        // Hitpoint indicator bar and text
        Rectangle indicatorRect = bgRect;
        indicatorRect.width *= combat.hitPoints / combat.hitPointsMax;
        DrawRectangleRec(indicatorRect, RED);
        DrawTextFont(s_font, hpText, hpRect.x, hpRect.y, 0, 0, fontSize, WHITE);
    }

    // Draw label
    DrawTextFont(s_font, nameText, nameRect.x, nameRect.y, 0, 0, fontSize, WHITE);

    if (combat.hitPointsMax) {
        DrawRectangleLinesEx(bgRect, 1, BLACK);
    }
}

void HealthBar::Dialog(const Vector2 &topCenter, const char *text)
{
    //const int fontSize = s_font.baseSize;
    //assert(fontSize);

    //Vector2 pad{ 16.0f, 8.0f };

    //Rectangle rect{};
    //Vector2 measure = MeasureTextEx(s_font, text, (float)fontSize, 1.0f);
    //rect.x = topCenter.x - measure.x / 2.0f;
    //rect.y = topCenter.y - measure.y;
    //rect.width = measure.x;
    //rect.height = measure.y;

    //Rectangle bgRect{};
    //bgRect.x = rect.x - pad.x;
    //bgRect.y = rect.y - pad.y;
    //bgRect.width = rect.width + pad.x * 2.0f;
    //bgRect.height = rect.height + pad.y * 2.0f;

    //// Draw background
    //DrawRectangleRec(bgRect, Fade(BLACK, 0.5f));
    //DrawTextFont(s_font, text, rect.x, rect.y, 0, 0, fontSize, WHITE);
    //DrawRectangleLinesEx(bgRect, 3, { 125, 108, 63, 255 });
}