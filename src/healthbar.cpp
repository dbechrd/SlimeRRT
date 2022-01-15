#include "healthbar.h"
#include <cmath>
#include <cassert>

Font HealthBar::s_font;

void HealthBar::SetFont(const Font font)
{
    HealthBar::s_font = font;
}

void HealthBar::Draw(int fontSize, const Vector3i &topCenter, uint32_t id, const char *name, int hitPoints, int maxHitPoints)
{
    assert(HealthBar::s_font.baseSize);

    Vector2i pad{ 4, 2 };
    int x = topCenter.x;
    int y = (topCenter.y - topCenter.z) - 10;

    const char *nameText = name ? TextFormat(name) : nullptr;
    int nameTextWidth = MeasureText(nameText, fontSize);

    Recti nameTextRect{};
    if (nameText) {
        nameTextRect.x = x - nameTextWidth / 2; // + nameTextRect.width % 2;
        nameTextRect.y = y - fontSize - 4 - fontSize;
        nameTextRect.height = fontSize;
        nameTextRect.width = nameTextWidth;
    } else {
        nameTextRect.x = INT_MAX;
        nameTextRect.y = INT_MAX;
    }

    const char *hpText = TextFormat("HP: %d / %d [%u]", hitPoints, maxHitPoints, id);
    int hpTextWidth = MeasureText(hpText, fontSize);
    Recti hpTextRect{};
    hpTextRect.x = x - hpTextWidth / 2;
    hpTextRect.y = y - fontSize;
    hpTextRect.width = hpTextWidth;
    hpTextRect.height = fontSize;

    Recti barBg{};
    barBg.x = MIN(hpTextRect.x, nameTextRect.x) - pad.x;
    barBg.y = hpTextRect.y - pad.y;
    barBg.width  = MAX(hpTextRect.width, nameTextRect.width) + pad.x * 2;
    barBg.height = hpTextRect.height + pad.y * 2;

    Recti barFg = barBg;
    barFg.width = (int)(barFg.width * ((float)hitPoints / maxHitPoints));

    DrawRectangle(barBg.x, barBg.y, barBg.width, barBg.height, DARKGRAY);
    DrawRectangle(barFg.x, barFg.y, barFg.width, barFg.height, RED);
    DrawRectangleLines(barBg.x, barBg.y, barBg.width, barBg.height, BLACK);
    DrawTextFont(s_font, nameText, nameTextRect.x, nameTextRect.y, 0, 0, fontSize, WHITE);
    DrawTextFont(s_font, hpText, hpTextRect.x, hpTextRect.y, 0, 0, fontSize, WHITE);
}