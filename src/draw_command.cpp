#include "draw_command.h"
#include "item_world.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

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
    if (cullEnabled && drawable.Cull(cullRect)) {
        return;
    }
#endif

    size_t size = sortedCommands.size();
    sortedCommands.resize(size + 1);
    const float depthA = drawable.Depth();
    int j;
    // NOTE: j is signed because it terminates at -1
    for (j = (int)size - 1; j >= 0; j--) {
        assert(sortedCommands[j].drawable);
        const float depthB = sortedCommands[j].drawable->Depth();
        if (depthB <= depthA) {
            break;
        }
        sortedCommands[(size_t)j + 1].drawable = sortedCommands[j].drawable;
    }
    sortedCommands[(size_t)j + 1].drawable = &drawable;
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
        assert(cmd.drawable);
        cmd.drawable->Draw();
#endif
    }

    sortedCommands.clear();
}