#include "item_world.h"

int ItemWorld::Depth(void) const
{
    const int depth = body.position.y;
    return depth;
}

bool ItemWorld::Cull(const Recti& cullRect) const
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
    const Vector3i bodyBC = body.BottomCenter();
    cull = !CheckCollisionCircleRecti({ bodyBC.x, bodyBC.y }, 10, cullRect);
#endif

    return cull;
}

void ItemWorld::Draw(void) const
{
    if (0 && sprite.spriteDef) {
        sprite_draw_body(sprite, body, WHITE);
    } else {
        Vector3i pos = body.BottomCenter();
        DrawRectangle(pos.x, pos.y, 20, 20, PURPLE);
    }
}