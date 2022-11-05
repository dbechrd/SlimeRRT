#include "draw_command.h"
#include <vector>

void DrawList::EnableCulling(const Rectangle &rect)
{
    cullRect = rect;
    cullEnabled = true;
}

void DrawList::DisableCulling(void)
{
    cullRect = {};
    DLB_ASSERT(cullRect.width == 0);
    cullEnabled = false;
}

void DrawList::Push(Drawable &drawable, float depth, bool cull, Vector2 at)
{
    // TODO: Check this before calling push
    //if (!drawable.sprite.spriteDef) {
    //    return;
    //}

#if CL_CULL_ON_PUSH
    if (cullEnabled && cull) {
        return;
    }
#endif

    size_t size = sortedCommands.size();
    sortedCommands.resize(size + 1);
    int j;
    // NOTE: j is signed because it terminates at -1
    for (j = (int)size - 1; j >= 0; j--) {
        if (depth > sortedCommands[j].depth) {
            break;
        }
        sortedCommands[(size_t)j + 1] = sortedCommands[j];
    }
    sortedCommands[(size_t)j + 1] = { &drawable, depth, cull, at };
}

void DrawList::Flush(World &world)
{
    if (sortedCommands.empty()) {
        return;
    }

    for (const DrawCommand &cmd : sortedCommands) {
#if !CL_CULL_ON_PUSH
        if (!cullEnabled || !cmd.cull) {
            cmd.drawable->Draw();
        }
#else
        DLB_ASSERT(cmd.drawable);
        cmd.drawable->Draw(world, cmd.at);
#endif
    }

    sortedCommands.clear();
}