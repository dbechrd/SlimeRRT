#pragma once
#include "raylib/raylib.h"
#include <vector>

enum DrawableType {
    Drawable_Particle,
    Drawable_Player,
    Drawable_Slime,
    Drawable_Count,
};

struct Drawable {
    DrawableType type;
    union {
        const struct Particle *particle;
        const struct Player *player;
        const struct Slime *slime;
    };
};

struct DrawCommand {
    Drawable drawable;
};

struct DrawList {
    void EnableCulling(const Rectangle &rect);  // must be enabled before calling push()
    void DisableCulling();
    void Push(const Drawable &drawable);
    void Flush();

    static void DrawList::RegisterTypes();

private:
    bool cullEnabled;
    Rectangle cullRect;
    std::vector<DrawCommand> sortedCommands;

    static struct DrawFn Methods[Drawable_Count];
    static void DrawList::RegisterType(DrawableType type, const DrawFn &drawFn);

    static float Drawable_Depth(const Drawable &drawable);
    static bool Drawable_Cull(const Drawable &drawable, const Rectangle &cullRect);
    static void Drawable_Draw(const Drawable &drawable);
};

