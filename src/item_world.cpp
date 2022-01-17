#include "item_world.h"

Vector3 ItemWorld::WorldCenter(void) const
{
    const Vector3 worldC = v3_add(body.WorldPosition(), sprite_center(sprite));
    return worldC;
}

Vector3 ItemWorld::WorldTopCenter(void) const
{
    const Vector3 worldTopC = v3_add(body.WorldPosition(), sprite_top_center(sprite));
    return worldTopC;
}

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

void ItemWorld::Update(double dt)
{
    if (stack.id != namedStack.id || stack.count != namedStack.count) {
        const char *itemName = Catalog::g_items.Name(stack.id);
        snprintf(name, sizeof(name), "%s (%u)", itemName, stack.count);
        namedStack.id = stack.id;
        namedStack.count = stack.count;
    }
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

        Vector3 topCenter = WorldTopCenter();
        HealthBar::Draw(10, { topCenter.x, topCenter.y - topCenter.z }, name, 0, 0);
    }
}