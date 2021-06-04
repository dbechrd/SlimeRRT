#pragma once
#include "raylib.h"

enum DrawableType {
    DrawableType_Particle,
    DrawableType_Player,
    DrawableType_Slime,
    DrawableType_Count
};

struct DrawCommand {
    DrawableType type;
    const void *drawable;
};

void draw_commands_init             (void);
void draw_commands_free             (void);
void draw_commands_enable_culling   (const Rectangle view);                    // must be enabled before calling push()
void draw_commands_disable_culling  (void);                                    // disable culling
void draw_command_push              (DrawableType type, const void *drawable); // culls, queues and sorts draw calls
void draw_commands_flush            (void);                                    // flushes command buffer