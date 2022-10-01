#pragma once
#include "direction.h"
#include "string_view.h"
#include "raylib/raylib.h"
#include <vector>

struct SpriteFrame {
    StringView name   {};  // name of frame
    int        x      {};  // x position (left)
    int        y      {};  // y position (top)
    int        width  {};  // width of frame
    int        height {};  // height of frame
};

#define SPRITEANIM_MAX_FRAMES 16

struct Spritesheet;

struct SpriteAnim {
    StringView    name        {};  // name of animation
    Spritesheet * spritesheet {};  // parent spritesheet
    size_t        frameCount  {};  // 0 = not used, 1 = static sprite, > 1 = animation frame count
    int           frames[SPRITEANIM_MAX_FRAMES]{};  // frame indices (spritesheet->frames)
};

struct SpriteDef {
    StringView          name        {};  // name of sprite
    const Spritesheet * spritesheet {};  // parent spritesheet
    int animations[(int)Direction::Count]{};  // animation index (spritesheet->animations)

    SpriteDef(const Spritesheet *spritesheet);

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