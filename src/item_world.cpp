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
    assert((int)stack.itemType);
    assert(stack.count);
    if (stack.itemType != namedStack.itemType || stack.count != namedStack.count) {
        const char *itemName = stack.Name();
        if (stack.count > 1) {
            snprintf(name, sizeof(name), "%s (%u)", itemName, stack.count);
        } else {
            snprintf(name, sizeof(name), "%s", itemName);
        }
        namedStack.itemType = stack.itemType;
        namedStack.count = stack.count;
    }
}

void ItemWorld::Draw(World &world)
{
    if (pickedUpAt) {
        DrawCircleV(body.VisualPosition(), 5.0f, RED);
    } else {
        if (sprite.spriteDef) {
            sprite_draw_body(sprite, body, WHITE);
        } else {
            const Texture tex = Catalog::g_items.Tex();
            const int texItemsWide = tex.width / 32;

            const int texIdx = (int)stack.itemType;
            const ImVec2 size = ImVec2(32.0f, 32.0f);
            const float texX = (texIdx % texItemsWide) * size.x;
            const float texY = (texIdx / texItemsWide) * size.y;

            const Rectangle srcRect{ texX, texY, 32.0f, 32.0f };
            const Vector2 pos = body.VisualPosition();
            const Vector2 dstPos = { pos.x - 16.0f, pos.y - 16.0f };
            DrawTextureRec(tex, srcRect, dstPos, WHITE);

            //DrawCircleV(body.VisualPosition(), 10.0f, PURPLE);
        }

        Vector3 topCenter = WorldTopCenter();
        HealthBar::Draw({ topCenter.x, topCenter.y - topCenter.z }, name, {});
    }
}