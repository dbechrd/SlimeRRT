#include "draw_command.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef float Drawable_FnDepth(const Drawable &drawable);
typedef bool  Drawable_FnCull(const Drawable &drawable, const Rectangle &cullRect);
typedef void  Drawable_FnDraw(const Drawable &drawable);

struct DrawableDef {
    Drawable_FnDepth *Depth;
    Drawable_FnCull *Cull;
    Drawable_FnDraw *Draw;
};

DrawableDef DrawList::registry[(size_t)DrawableType::Count];

void DrawList::RegisterType(DrawableType type, const DrawableDef &def)
{
    registry[(size_t)type] = def;
}

void DrawList::RegisterTypes()
{
    RegisterType(DrawableType::Particle, { particle_depth, particle_cull, particle_draw });
    RegisterType(DrawableType::Player, { Player_Depth, Player_Cull, Player_Draw });
    RegisterType(DrawableType::Slime, { Slime_Depth, Slime_Cull, Slime_Draw });
}

void DrawList::EnableCulling(const Rectangle &rect)
{
    cullRect = rect;
    cullEnabled = true;
}

void DrawList::DisableCulling()
{
    cullRect = {};
    assert(cullRect.width == 0);
    cullEnabled = false;
}

void DrawList::Push(const Drawable &drawable)
{
    // TODO: Check this before calling push
    //if (!drawable.sprite.spriteDef) {
    //    return;
    //}

#if CULL_ON_PUSH
    if (cullEnabled && registry[(size_t)drawable.type].Cull(drawable, cullRect)) {
        return;
    }
#endif

    // TODO: Implement fixed size grid of 8x8 cells
    // TODO: Only sort items that are in the same broadphase cell
    // TODO: Research quad tree vs. AABB

    size_t size = sortedCommands.size();
    sortedCommands.resize(size + 1);
    const float depthA = Drawable_Depth(drawable);
    int j;
    // NOTE: j is signed because it terminates at -1
    for (j = (int)size - 1; j >= 0; j--) {
        const float depthB = Drawable_Depth(sortedCommands[j].drawable);
        if (depthB <= depthA) {
            break;
        }
        sortedCommands[(size_t)j + 1].drawable = sortedCommands[j].drawable;
    }
    sortedCommands[(size_t)j + 1].drawable = drawable;
}

void DrawList::Flush()
{
    if (sortedCommands.empty()) {
        return;
    }

    for (const DrawCommand &cmd : sortedCommands) {
#if !CULL_ON_PUSH
        if (!cullEnabled || !cmd.drawable.Cull()) {
            cmd.drawable->Draw();
        }
#else
        Drawable_Draw(cmd.drawable);
#endif
    }

    sortedCommands.clear();
}

float DrawList::Drawable_Depth(const Drawable &drawable)
{
    return registry[(size_t)drawable.type].Depth(drawable);
}

bool DrawList::Drawable_Cull(const Drawable &drawable, const Rectangle &cullRect)
{
    return registry[(size_t)drawable.type].Cull(drawable, cullRect);
}

void DrawList::Drawable_Draw(const Drawable &drawable)
{
    registry[(size_t)drawable.type].Draw(drawable);
}