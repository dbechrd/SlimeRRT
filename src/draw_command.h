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
    virtual float Depth(void) const = 0;
    virtual bool Cull(const Rectangle& cullRect) const = 0;
    virtual void Draw(World &world) = 0;
};

struct DrawCommand {
    Drawable *drawable {};
};

struct DrawList {
    void EnableCulling(const Rectangle &rect);  // must be enabled before calling push()
    void DisableCulling();
    void Push(Drawable &drawable);
    void Flush(World &world);

    bool      cullEnabled {};
    Rectangle cullRect    {};
private:
    std::vector<DrawCommand> sortedCommands {};
};

