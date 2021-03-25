#include "draw_command.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t commandCapacity;
static size_t commandCount;
static DrawCommand *sortedCommands;

void draw_commands_init(void)
{
    // HACK: reserve enough draw commands for worst case
    // TODO: dynamically grow command list
    const size_t MAX_PLAYERS = 1;
    commandCapacity = MAX_PARTICLES + MAX_SLIMES + MAX_PLAYERS;
    sortedCommands = calloc(commandCapacity, sizeof(*sortedCommands));
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
            depth = particle_depth(cmd->drawable);
            break;
        }
        case DrawableType_Player: {
            depth = player_depth(cmd->drawable);
            break;
        }
        case DrawableType_Slime: {
            depth = slime_depth(cmd->drawable);
            break;
        }
    }
    return depth;
}

void draw_command_push(DrawableType type, const void *drawable)
{
    assert(drawable);
    assert(commandCount < commandCapacity);

    DrawCommand cmd = { 0 };
    cmd.type = type;
    cmd.drawable = drawable;

    // TODO: Implement fixed size grid of 8x8 cells
    // TODO: Only sort items that are in the same broadphase cell
    // TODO: Research quad tree vs. AABB

    switch (type) {
        case DrawableType_Particle: {
            sortedCommands[commandCount] = cmd;
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

void draw_commands_flush(void)
{
    if (!commandCount) {
        return;
    }

    for (size_t i = 0; i < commandCount; i++) {
        switch (sortedCommands[i].type) {
            case DrawableType_Particle: {
                particle_draw(sortedCommands[i].drawable);
                break;
            }
            case DrawableType_Player: {
                player_draw(sortedCommands[i].drawable);
                break;
            }
            case DrawableType_Slime: {
                slime_draw(sortedCommands[i].drawable);
                break;
            }
        }
    }

    memset(sortedCommands, 0, commandCount * sizeof(*sortedCommands));
    commandCount = 0;
}