#include "draw_command.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef float DrawableDepthFn(const Drawable &drawable);
typedef bool  DrawableCullFn(const Drawable &drawable, const Rectangle &cullRect);
typedef void  DrawableDrawFn(const Drawable &drawable);

struct DrawFn {
    DrawableDepthFn *Depth;
    DrawableCullFn *Cull;
    DrawableDrawFn *Draw;
};

DrawFn DrawList::Methods[Drawable_Count];

void DrawList::RegisterType(DrawableType type, const DrawFn &drawFn)
{
    Methods[type] = drawFn;
}

void DrawList::RegisterTypes()
{
    RegisterType(Drawable_Particle, { particle_depth, particle_cull, particle_draw });
    RegisterType(Drawable_Player, { Player_Depth, Player_Cull, Player_Draw });
    RegisterType(Drawable_Slime, { Slime_Depth, Slime_Cull, Slime_Draw });
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
    if (cullEnabled && Methods[drawable.type].Cull(drawable, cullRect)) {
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
    return Methods[drawable.type].Depth(drawable);
}

bool DrawList::Drawable_Cull(const Drawable &drawable, const Rectangle &cullRect)
{
    return Methods[drawable.type].Cull(drawable, cullRect);
}

void DrawList::Drawable_Draw(const Drawable &drawable)
{
    Methods[drawable.type].Draw(drawable);
}