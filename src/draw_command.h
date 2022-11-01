#pragma once
#include "raylib/raylib.h"
#include <vector>

struct World;

//enum class DrawableType {
//    Particle,
//    Player,
//    Enemy,
//    WorldItem,
//    Count,
//};

class Drawable {
public:
    virtual void Draw(World &world, Vector2 at) const = 0;
};

struct DrawCommand {
    const Drawable *drawable {};
    float    depth     {};
    bool     cull      {};
    Vector2  at        {};  // for things that don't know where they are by themselves (e.g. tile-based entities)

    DrawCommand(void) {};

    DrawCommand(const Drawable *drawable, float depth, bool cull = false, Vector2 at = { 0, 0 })
        : drawable(drawable), at(at), depth(depth), cull(cull) {};
};

struct DrawList {
    void EnableCulling(const Rectangle &rect);  // must be enabled before calling push()
    void DisableCulling();
    void Push(const Drawable &drawable, float depth, bool cull = false, Vector2 at = { 0, 0 });
    void Flush(World &world);

    bool      cullEnabled {};
    Rectangle cullRect    {};
private:
    std::vector<DrawCommand> sortedCommands {};
};

