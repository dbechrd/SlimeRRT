#pragma once
#include "drawable.h"
#include "raylib.h"
#include <vector>

struct DrawCommand {
    const Drawable *drawable{};

    DrawCommand &operator=(const DrawCommand &other);
};

struct DrawList {
    void EnableCulling(const Rectangle view);  // must be enabled before calling push()
    void DisableCulling();
    void Push(const Drawable &drawable);
    void Flush();

private:
    std::vector<DrawCommand> sortedCommands{};
    bool cullEnabled{};
    Rectangle cullRect{};
};
