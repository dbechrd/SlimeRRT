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
    assert(fontSize == HealthBar::s_font.baseSize);  // not necessary, just testing

    const Vector2i pad{ 4, 2 };
    const int nameOffset = 4;
    const int x = topCenter.x;
    const int y = (topCenter.y - topCenter.z) - PIXELS_TO_UNITS(10);

    const char *nameText = name ? TextFormat(name) : nullptr;
    int nameTextWidth = MeasureText(nameText, fontSize);

    Recti nameTextRect{};
    if (nameText) {
        nameTextRect.x = x - PIXELS_TO_UNITS((nameTextWidth + 1) / 2);
        nameTextRect.y = y - PIXELS_TO_UNITS(fontSize) - (PIXELS_TO_UNITS(fontSize) + PIXELS_TO_UNITS(nameOffset));
        nameTextRect.height = fontSize;
        nameTextRect.width = nameTextWidth;
    } else {
        nameTextRect.x = INT_MAX;
        nameTextRect.y = INT_MAX;
    }

    const char *hpText = TextFormat("HP: %d / %d [%u]", hitPoints, maxHitPoints, id);
    int hpTextWidth = MeasureText(hpText, fontSize);
    Recti hpTextRect{};
    hpTextRect.x = x - PIXELS_TO_UNITS((hpTextWidth + 1) / 2);
    hpTextRect.y = y - PIXELS_TO_UNITS(fontSize);
    hpTextRect.width = hpTextWidth;
    hpTextRect.height = fontSize;

    Recti barBg{};
    barBg.x = MIN(hpTextRect.x, nameTextRect.x) - PIXELS_TO_UNITS(pad.x);
    barBg.y = hpTextRect.y - PIXELS_TO_UNITS(pad.y);
    barBg.width  = MAX(hpTextRect.width, nameTextRect.width) + pad.x * 2;
    barBg.height = hpTextRect.height + pad.y * 2;

    Recti barFg = barBg;
    barFg.width = (int)(barFg.width * ((float)hitPoints / maxHitPoints));

    DrawRectangleRec({ UNITS_TO_PIXELS((float)barBg.x), UNITS_TO_PIXELS((float)barBg.y), (float)barBg.width, (float)barBg.height }, DARKGRAY);
    DrawRectangleRec({ UNITS_TO_PIXELS((float)barFg.x), UNITS_TO_PIXELS((float)barFg.y), (float)barFg.width, (float)barFg.height }, RED);
    DrawRectangleLinesEx({ UNITS_TO_PIXELS((float)barBg.x), UNITS_TO_PIXELS((float)barBg.y), (float)barBg.width, (float)barBg.height }, 1.0f, BLACK);
    DrawTextFont(s_font, nameText, UNITS_TO_PIXELS((float)nameTextRect.x), UNITS_TO_PIXELS((float)nameTextRect.y), 0, 0, fontSize, WHITE);
    DrawTextFont(s_font, hpText, UNITS_TO_PIXELS((float)hpTextRect.x), UNITS_TO_PIXELS((float)hpTextRect.y), 0, 0, fontSize, WHITE);
}