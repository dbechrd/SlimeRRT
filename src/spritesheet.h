#pragma once
#include <stdint.h>

// Note: Size determined by sprite, must all be the same
typedef struct SpriteFrame {
    int x;
    int y;
    int width;
    int height;
} SpriteFrame;

typedef struct Sprite {
    struct Spritesheet *spritesheet;
    size_t frameCount;      // note 1 for static sprites
    SpriteFrame *frames;    // frames for this sprite's animation
} Sprite;

typedef struct Spritesheet {
    size_t spriteCount;
    Sprite *sprites;        // every sprite in the sheet
    struct Texture *texture;
} Spritesheet;