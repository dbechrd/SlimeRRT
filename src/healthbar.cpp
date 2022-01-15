#include "healthbar.h"
#include <cmath>
#include <cassert>

Font HealthBar::s_font;

void HealthBar::SetFont(const Font font)
{
    HealthBar::s_font = font;
}

void HealthBar::Draw(int fontSize, const Sprite &sprite, const Body3D &body, const char *name, int hitPoints, int maxHitPoints)
{
    assert(HealthBar::s_font.baseSize);

    Vector2i pad{ 4, 2 };
    Vector3i topCenter = sprite_world_top_center(sprite, body.position, sprite.scale);
    int x = topCenter.x;
    int y = (topCenter.y - topCenter.z) - 10;
    //float y = topCenter.y - 10.0f;

    const char *hpText = TextFormat("HP: %d / %d", hitPoints, maxHitPoints);
    const char *nameText = name ? TextFormat(name) : nullptr;

    Recti nameTextRect{};
    if (nameText) {
        nameTextRect.width = MeasureText(nameText, fontSize);
        nameTextRect.height = fontSize;
        nameTextRect.x = x - nameTextRect.width / 2 + nameTextRect.width % 2;
        nameTextRect.y = y - fontSize - 4 - fontSize;
    } else {
        nameTextRect.x = INT_MAX;
        nameTextRect.y = INT_MAX;
    }

    Recti hpTextRect{};
    hpTextRect.x = x - hpTextRect.width / 2 + hpTextRect.width % 2;
    hpTextRect.y = y - fontSize;
    hpTextRect.width = MeasureText(hpText, fontSize);
    hpTextRect.height = fontSize;

    Recti barBg{};
    barBg.x = MIN(hpTextRect.x, nameTextRect.x) - pad.x;
    barBg.y = hpTextRect.y - pad.y;
    barBg.width  = MAX(hpTextRect.width, nameTextRect.width) + pad.x * 2;
    barBg.height = hpTextRect.height + pad.y * 2;

    Recti barFg = barBg;
    barFg.width *= (int)((float)hitPoints / maxHitPoints);

    DrawRectangle(barBg.x, barBg.y, barBg.width, barBg.height, DARKGRAY);
    DrawRectangleLines(barBg.x, barBg.y, barBg.width, barBg.height, BLACK);
    DrawRectangle(barFg.x, barFg.y, barFg.width, barFg.height, RED);
    DrawTextFont(s_font, nameText, nameTextRect.x, nameTextRect.y, fontSize, WHITE);
    DrawTextFont(s_font, hpText, hpTextRect.x, hpTextRect.y, fontSize, WHITE);
}