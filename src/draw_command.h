#pragma once
#include "raylib/raylib.h"
#include <vector>

struct World;

enum class DrawableType {
    Particle,
    Player,
    Slime,
    ItemWorld,
    Count,
};

struct Drawable {
    virtual float Depth() const = 0;
    virtual bool Cull(const Rectangle& cullRect) const = 0;
    virtual void Draw(World &world) const = 0;
};

struct DrawCommand {
    const Drawable *drawable {};
};

struct DrawList {
    void EnableCulling(const Rectangle &rect);  // must be enabled before calling push()
    void DisableCulling();
    void Push(const Drawable &drawable);
    void Flush(World &world);

    bool      cullEnabled {};
    Rectangle cullRect    {};
private:
    std::vector<DrawCommand> sortedCommands {};
};

