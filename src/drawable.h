#pragma once
#include "raylib/raylib.h"
#include "sprite.h"

struct Drawable {
    Sprite sprite{};

    virtual float Depth() const = 0;
    virtual bool Cull(const Rectangle &cullRect) const = 0;
	virtual void Draw() const = 0;
};