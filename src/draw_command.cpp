#include "draw_command.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

static size_t commandCapacity;
static size_t commandCount;
static DrawCommand *sortedCommands;
static bool cullEnabled;
static Rectangle cullRect;

void draw_commands_init(void)
{
    // HACK: reserve enough draw commands for worst case
    // TODO: dynamically grow command list
    const size_t MAX_PLAYERS = 1;
    commandCapacity = MAX_PARTICLES + MAX_SLIMES + MAX_PLAYERS;
    sortedCommands = (DrawCommand *)calloc(commandCapacity, sizeof(*sortedCommands));
}

void draw_commands_free(void)
{
    free(sortedCommands);
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
            depth = player_depth((const Player *)cmd->drawable);
            break;
        }
        case DrawableType_Slime: {
            depth = slime_depth((const Slime *)cmd->drawable);
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
            cull = player_cull((const Player *)cmd->drawable, cullRect);
            break;
        }
        case DrawableType_Slime: {
            cull = slime_cull((const Slime *)cmd->drawable, cullRect);
            break;
        }
    }

    return cull;
}

void draw_command_push(DrawableType type, const void *drawable)
{
    assert(drawable);
    assert(commandCount < commandCapacity);

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
            const float depthA = draw_command_depth(&cmd);
            int j;
            for (j = (int)commandCount - 1; j >= 0; j--) {
                const float depthB = draw_command_depth(&sortedCommands[j]);
                if (depthB <= depthA) {
                    break;
                }
                sortedCommands[j + 1] = sortedCommands[j];
            }
            sortedCommands[j + 1] = cmd;
        }
    }

    commandCount++;
}

static void draw_command_draw(const DrawCommand *cmd)
{
    switch (cmd->type) {
        case DrawableType_Particle: {
            particle_draw((const Particle *)cmd->drawable);
            break;
        }
        case DrawableType_Player: {
            player_draw((const Player *)cmd->drawable);
            break;
        }
        case DrawableType_Slime: {
            slime_draw((const Slime *)cmd->drawable);
            break;
        }
    }
}

void draw_commands_flush(void)
{
    if (!commandCount) {
        return;
    }

    for (size_t i = 0; i < commandCount; i++) {
        const DrawCommand *cmd = &sortedCommands[i];
#if !CULL_ON_PUSH
        if (!cullEnabled || !draw_command_cull(cmd)) {
            draw_command_draw(cmd);
        }
#else
        draw_command_draw(cmd);
#endif
    }

    memset(sortedCommands, 0, commandCount * sizeof(*sortedCommands));
    commandCount = 0;
}