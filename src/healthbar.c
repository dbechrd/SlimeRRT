#include "healthbar.h"
#include <math.h>

static Font healthbarFont;

void healthbars_set_font(const Font font)
{
    healthbarFont = font;
}

void healthbar_draw(int fontSize, const Sprite *sprite, const Body3D *body, float hitPoints, float maxHitPoints)
{
    Vector3 topCenter = sprite_world_top_center(sprite, body->position, sprite->scale);
    float x = topCenter.x;
    float y = (topCenter.y - topCenter.z) - 10.0f;
    //float y = topCenter.y - 10.0f;

    const float hpPercent = hitPoints / maxHitPoints;
    const char *hpText = TextFormat("HP: %.02f / %.02f", hitPoints, maxHitPoints);

    Rectangle hpRect = { 0 };
    hpRect.width = (float)MeasureText(hpText, fontSize);
    hpRect.height = (float)fontSize;
    hpRect.x = x - ceilf(hpRect.width / 2.0f);
    hpRect.y = y - fontSize;

    Vector2 pad = { 4.0f, 2.0f };
    Rectangle bgRect = { 0 };
    bgRect.x = hpRect.x - pad.x;
    bgRect.y = hpRect.y - pad.y;
    bgRect.width  = hpRect.width  + pad.x * 2.0f;
    bgRect.height = hpRect.height + pad.y * 2.0f;

    Rectangle indicatorRect = bgRect;
    indicatorRect.width *= hpPercent;

    // Draw hitpoint indicators
    DrawRectangleRec(bgRect, DARKGRAY);
    DrawRectangleRec(indicatorRect, RED);
    DrawRectangleLinesEx(bgRect, 1, BLACK);
    DrawTextFont(healthbarFont, hpText, hpRect.x, hpRect.y, fontSize, WHITE);
}