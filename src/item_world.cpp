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
    if (pickedUpAt) {
        DrawCircleV(body.VisualPosition(), 20.0f, RED);
    } else {
        if (sprite.spriteDef) {
            sprite_draw_body(sprite, body, WHITE);
        } else {
            DrawCircleV(body.VisualPosition(), 20.0f, PURPLE);
        }
    }
}