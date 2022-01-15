#include "healthbar.h"
#include <cmath>
#include <cassert>

Font HealthBar::s_font;

void HealthBar::SetFont(const Font font)
{
    HealthBar::s_font = font;
}

void HealthBar::Draw(int fontSize, const Vector2 &topCenter, const char *name, float hitPoints, float maxHitPoints)
{
    assert(HealthBar::s_font.baseSize);

    Vector2 pad{ 4.0f, 2.0f };
    float x = topCenter.x;
    float y = topCenter.y - 10.0f;
    //float y = topCenter.y - 10.0f;

    const float hpPercent = hitPoints / maxHitPoints;
    //const char *hpText = TextFormat("HP: %.02f / %.02f", hitPoints, hitPointsMax);
    const char *hpText = TextFormat("HP: %.f / %.f", hitPoints, maxHitPoints);
    const char *nameText = name ? TextFormat(name) : nullptr;

    Rectangle hpRect{};
    hpRect.width = (float)MeasureText(hpText, fontSize);
    hpRect.height = (float)fontSize;
    hpRect.x = x - ceilf(hpRect.width / 2.0f);
    hpRect.y = y - fontSize;

    Rectangle nameRect{};
    if (nameText) {
        nameRect.width = (float)MeasureText(nameText, fontSize);
        nameRect.height = (float)fontSize;
        nameRect.x = x - ceilf(nameRect.width / 2.0f);
        nameRect.y = y - fontSize - 4.0f - fontSize;
    } else {
        nameRect.x = FLT_MAX;
        nameRect.y = FLT_MAX;
    }

    Rectangle bgRect{};
    bgRect.x = MIN(hpRect.x, nameRect.x) - pad.x;
    bgRect.y = hpRect.y - pad.y;
    bgRect.width  = MAX(hpRect.width, nameRect.width) + pad.x * 2.0f;
    bgRect.height = hpRect.height + pad.y * 2.0f;

    Rectangle indicatorRect = bgRect;
    indicatorRect.width *= hpPercent;

    // Draw hitpoint indicators
    DrawRectangleRec(bgRect, DARKGRAY);
    DrawRectangleRec(indicatorRect, RED);
    DrawTextFont(s_font, nameText, nameRect.x, nameRect.y, 0, 0, fontSize, WHITE);
    DrawTextFont(s_font, hpText, hpRect.x, hpRect.y, 0, 0, fontSize, WHITE);
    DrawRectangleLinesEx(bgRect, 1, BLACK);
}