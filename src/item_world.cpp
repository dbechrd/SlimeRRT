#include "item_world.h"

float ItemWorld::Depth(void) const
{
    const float depth = body.GroundPosition().y;
    return depth;
}

bool ItemWorld::Cull(const Rectangle& cullRect) const
{
    bool cull = false;

#if 0
    if (sprite.spriteDef) {
        cull = sprite_cull_body(sprite, body, cullRect);
    } else {
        const Vector2 bodyBC = body.BottomCenter();
        cull = !CheckCollisionCircleRec(bodyBC, sprite.scale, cullRect);
    }
#else
    const Vector2 bodyBC = body.VisualPosition();
    cull = !CheckCollisionCircleRec(bodyBC, 10.0f, cullRect);
#endif

    return cull;
}

void ItemWorld::Draw(void) const
{
    if (0 && sprite.spriteDef) {
        sprite_draw_body(sprite, body, WHITE);
    } else {
        Vector2 pos = body.VisualPosition();
        Vector2 size { 20.0f, 20.0f };
        DrawRectangleV(pos, size, PURPLE);
    }
}