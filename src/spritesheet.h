#pragma once
#include "direction.h"
#include "string_view.h"
#include "raylib/raylib.h"
#include <vector>

struct Spritesheet;

struct SpriteFrame : Drawable {
    StringView          name        {};  // name of frame
    const Spritesheet * spritesheet {};  // parent spritesheet
    int                 x           {};  // x position (left)
    int                 y           {};  // y position (top)
    int                 width       {};  // width of frame
    int                 height      {};  // height of frame

    SpriteFrame(const Spritesheet *spritesheet) : spritesheet(spritesheet) {}

    void Draw(World &world, Vector2 at) const;
};

#define SPRITEANIM_MAX_FRAMES 16

struct SpriteAnim {
    StringView          name        {};  // name of animation
    const Spritesheet * spritesheet {};  // parent spritesheet
    size_t              frameCount  {};  // 0 = not used, 1 = static sprite, > 1 = animation frame count
    int                 frames      [SPRITEANIM_MAX_FRAMES]{};  // frame indices (spritesheet->frames)

    SpriteAnim(const Spritesheet *spritesheet) : spritesheet(spritesheet) {}
};

struct SpriteDef {
    StringView          name        {};  // name of sprite
    const Spritesheet * spritesheet {};  // parent spritesheet
    int animations[(int)Direction::Count]{};  // animation index (spritesheet->animations)

    SpriteDef(const Spritesheet *spritesheet) : spritesheet(spritesheet) {}

private:
    SpriteDef() = default;
};

struct Spritesheet {
    Texture                  texture    {};  // spritesheet texture
    std::vector<SpriteFrame> frames     {};  // array of frames
    std::vector<SpriteAnim>  animations {};  // array of animations
    std::vector<SpriteDef>   sprites    {};  // array of sprites definitions
    unsigned int             bufLength  {};  // length of file buffer in memory
    char *                   buf        {};  // file buffer (needs to be freed with UnloadFileData())

    ~Spritesheet();
    ErrorType LoadFromFile(const char *filename);
    const SpriteDef *FindSprite(const char *name) const;

private:
    const char *LOG_SRC = "Spritesheet";
};
