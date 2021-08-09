#include "draw_command.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

static std::vector<DrawCommand> sortedCommands;
static bool cullEnabled;
static Rectangle cullRect;

void draw_commands_init(void)
{
    // HACK: reserve enough draw commands for worst case
    // TODO: dynamically grow command list
    const size_t MAX_PLAYERS = 1;
}

void draw_commands_free(void)
{
    sortedCommands.clear();
}

static float draw_command_depth(const DrawCommand *cmd)
{
    float depth = 0.0f;
    switch (cmd->type) {
        case DrawableType_Particle: {
            depth = particle_depth((const Particle *)cmd->drawable);
            break;
        }
        case DrawableType_Player: {
            depth = ((const Player *)cmd->drawable)->Depth();
            break;
        }
        case DrawableType_Slime: {
            depth = ((const Slime *)cmd->drawable)->Depth();
            break;
        }
    }
    return depth;
}

void draw_commands_enable_culling(const Rectangle rect)
{
    cullRect = rect;
    cullEnabled = true;
}

void draw_commands_disable_culling(void)
{
    cullRect = {};
    assert(cullRect.width == 0);
    cullEnabled = false;
}

static bool draw_command_cull(const DrawCommand *cmd)
{
    bool cull = false;

    switch (cmd->type) {
        case DrawableType_Particle: {
            cull = particle_cull((const Particle *)cmd->drawable, cullRect);
            break;
        }
        case DrawableType_Player: {
            cull = ((const Player *)cmd->drawable)->Cull(cullRect);
            break;
        }
        case DrawableType_Slime: {
            cull = ((const Slime *)cmd->drawable)->Cull(cullRect);
            break;
        }
    }

    return cull;
}

void draw_command_push(DrawableType type, const void *drawable)
{
    assert(drawable);

    DrawCommand cmd = {};
    cmd.type = type;
    cmd.drawable = drawable;

#if CULL_ON_PUSH
    if (cullEnabled && draw_command_cull(&cmd)) {
        return;
    }
#endif

    // TODO: Implement fixed size grid of 8x8 cells
    // TODO: Only sort items that are in the same broadphase cell
    // TODO: Research quad tree vs. AABB

    switch (type) {
        case DrawableType_Particle: {
#if 0
            sortedCommands[commandCount] = cmd;
#endif
        }
        default: {
            size_t size = sortedCommands.size();
            sortedCommands.resize(size + 1);
            const float depthA = draw_command_depth(&cmd);
            int j;
            // NOTE: j is signed because it terminates at -1
            for (j = (int)size - 1; j >= 0; j--) {
                const float depthB = draw_command_depth(&sortedCommands[j]);
                if (depthB <= depthA) {
                    break;
                }
                sortedCommands[(size_t)j + 1] = sortedCommands[j];
            }
            sortedCommands[(size_t)j + 1] = cmd;
        }
    }
}

static void draw_command_draw(const DrawCommand *cmd)
{
    switch (cmd->type) {
        case DrawableType_Particle: {
            particle_draw((const Particle *)cmd->drawable);
            break;
        }
        case DrawableType_Player: {
            ((const Player *)cmd->drawable)->Draw();
            break;
        }
        case DrawableType_Slime: {
            ((const Slime *)cmd->drawable)->Draw();
            break;
        }
    }
}

void draw_commands_flush(void)
{
    if (sortedCommands.empty()) {
        return;
    }

    for (const DrawCommand &cmd : sortedCommands) {
#if !CULL_ON_PUSH
        if (!cullEnabled || !draw_command_cull(cmd)) {
            draw_command_draw(cmd);
        }
#else
        draw_command_draw(&cmd);
#endif
    }

    sortedCommands.clear();
}