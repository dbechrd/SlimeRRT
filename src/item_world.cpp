#include "item_world.h"
#include "shadow.h"

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
    DLB_ASSERT(stack.uid);
    DLB_ASSERT(stack.count);
    const Item &item = g_item_db.Find(stack.uid);

    if (item.uid != namedStack.uid || stack.count != namedStack.count) {
        const char *itemName = item.Name();
        if (stack.count > 1) {
            snprintf(name, sizeof(name), "%s (%u)", itemName, stack.count);
        } else {
            snprintf(name, sizeof(name), "%s", itemName);
        }
        namedStack.uid = stack.uid;
        namedStack.count = stack.count;
    }
}

void ItemWorld::Draw(World &world)
{
    DLB_ASSERT(stack.uid);
    DLB_ASSERT(stack.count);
    const Item &item = g_item_db.Find(stack.uid);

    const Vector3 worldPos = body.WorldPosition();
    Shadow::Draw(worldPos, 16.0f * sprite.scale, 8.0f);

    if (pickedUpAt) {
        //DrawCircleV(body.VisualPosition(), 5.0f, RED);
    } else {
        if (sprite.spriteDef) {
            sprite_draw_body(sprite, body, WHITE);
        } else {
            const Texture tex = g_item_catalog.Tex();
            const int texItemsWide = tex.width / 32;

            const int texIdx = item.type;
            const float texX = (float)(texIdx % texItemsWide) * ITEM_W;
            const float texY = (float)(texIdx / texItemsWide) * ITEM_H;

            Rectangle srcRect{ texX, texY, 32.0f, 32.0f };
            const Vector2 bodyPos = body.VisualPosition();
            const Vector2 dstPos = { bodyPos.x - 16.0f, bodyPos.y - 16.0f };

#if 1
            DrawTextureRec(tex, srcRect, dstPos, WHITE);
#else
            const float rot = 0; //((sin(g_clock.now) + 1) * 0.5) * 360.0f;
            float wid = 16; //(float)sin(g_clock.now * 10.0) * 12.0f + 4.0f;

            const Vector2 pos = g_spycam.WorldToScreen(dstPos);
            Vector2 size{ ITEM_W, ITEM_H };
            //Vector2 size{ wid * 2, ITEM_H };

            ImDrawList *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddImage(
                (ImTextureID)(size_t)tex.id,
                { pos.x, pos.y },
                { pos.x + size.x, pos.y + size.y },
                { srcRect.x / tex.width, srcRect.y / tex.height },
                { (srcRect.x + srcRect.width) / tex.width, (srcRect.y + srcRect.height) / tex.height }
            );

            //if (wid < 0) {
            //    //srcRect.width *= -1;
            //    DrawTextureTiled(tex, srcRect, dstRect, { wid, 0 }, rot, -1.0f, WHITE);
            //} else {
            //    DrawTexturePro(tex, srcRect, dstRect, { wid, 0 }, rot, WHITE);
            //}
#endif
        }
        //DrawCircleV(body.VisualPosition(), 10.0f, PURPLE);

        Vector3 topCenter = WorldTopCenter();
        HealthBar::Draw({ topCenter.x, topCenter.y - topCenter.z }, name, {});
    }
}