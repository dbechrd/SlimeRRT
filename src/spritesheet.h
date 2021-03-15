#pragma once
#include "helpers.h"

typedef struct SpriteFrame {
    StringView name;  // name of frame
    int x;            // x position (left)
    int y;            // y position (top)
    int width;        // width of frame
    int height;       // height of frame
} SpriteFrame;

#define SPRITEANIM_MAX_FRAMES 16
typedef struct Spritesheet Spritesheet;

typedef struct SpriteAnim {
    StringView name;                    // name of animation
    Spritesheet *spritesheet;           // parent spritesheet
    size_t frameCount;                  // 0 = not used, 1 = static sprite, > 1 = animation frame count
    int frames[SPRITEANIM_MAX_FRAMES];  // frame indices (into spritesheet->frames)
} SpriteAnim;

typedef struct Sprite {
    StringView name;               // name of sprite
    Spritesheet *spritesheet;      // parent spritesheet
    int animations[Facing_Count];  // animation data for each direction
} Sprite;

typedef struct Spritesheet {
    Texture texture;        // spritesheet texture
    int frameCount;         // # of frames
    SpriteFrame *frames;    // array of frames
    int animationCount;     // # of animations
    SpriteAnim *animations; // array of animations
    int spriteCount;        // # of sprites
    Sprite *sprites;        // array of sprites
    unsigned int bufLength; // length of file buffer in memory
    unsigned char *buf;     // file buffer (needs to be freed with UnloadFileData())
} Spritesheet;

Spritesheet *LoadSpritesheet (const char *fileName);
void UnloadSpritesheet       (Spritesheet *spritesheet);