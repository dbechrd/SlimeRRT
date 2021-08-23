#include "draw_command.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

DrawCommand &DrawCommand::operator=(const DrawCommand &other)
{
    if (this != &other) {
        drawable = other.drawable;
    }
    return *this;
}

void DrawList::EnableCulling(const Rectangle rect)
{
    cullRect = rect;
    cullEnabled = true;
}

void DrawList::DisableCulling(void)
{
    cullRect = {};
    assert(cullRect.width == 0);
    cullEnabled = false;
}

void DrawList::Push(const Drawable &drawable)
{
#if CULL_ON_PUSH
    if (cullEnabled && drawable.Cull(cullRect)) {
        return;
    }
#endif

    // TODO: Implement fixed size grid of 8x8 cells
    // TODO: Only sort items that are in the same broadphase cell
    // TODO: Research quad tree vs. AABB

    size_t size = sortedCommands.size();
    sortedCommands.resize(size + 1);
    const float depthA = drawable.Depth();
    int j;
    // NOTE: j is signed because it terminates at -1
    for (j = (int)size - 1; j >= 0; j--) {
        const float depthB = sortedCommands[j].drawable->Depth();
        if (depthB <= depthA) {
            break;
        }
        sortedCommands[(size_t)j + 1] = sortedCommands[j];
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
        cmd.drawable->Draw();
#endif
    }

    sortedCommands.clear();
}