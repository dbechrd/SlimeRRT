#pragma once
#include "raylib/raylib.h"
#include <vector>

enum class DrawableType {
    Particle,
    Player,
    Slime,
    ItemWorld,
    Count,
};

struct Drawable {
    virtual int Depth() const = 0;
    virtual bool Cull(const Recti &cullRect) const = 0;
    virtual void Draw() const = 0;
};

struct DrawCommand {
    const Drawable *drawable {};
};

struct DrawList {
    void EnableCulling(const Recti &rect);  // must be enabled before calling push()
    void DisableCulling();
    void Push(const Drawable &drawable);
    void Flush();

    bool  cullEnabled {};
    Recti cullRect    {};
private:
    std::vector<DrawCommand> sortedCommands {};
};
