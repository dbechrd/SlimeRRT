#pragma once

typedef enum DrawableType {
    DrawableType_Particle,
    DrawableType_Player,
    DrawableType_Slime,
    DrawableType_Count
} DrawableType;

typedef struct DrawCommand {
    DrawableType type;
    const void *drawable;
} DrawCommand;

void draw_commands_init  (void);
void draw_commands_free  (void);
void draw_command_push   (DrawableType type, const void *drawable);
void draw_commands_flush (void);