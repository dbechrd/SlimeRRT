#include "spritesheet.h"
#include "body.h"
#include "helpers.h"
#include "raylib.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define ALPHA_LOWER \
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': \
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': \
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
#define ALPHA_UPPER \
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': \
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': \
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
#define ALPHA \
    ALPHA_LOWER \
    ALPHA_UPPER
#define NUM \
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
#define ALPHA_NUM \
    ALPHA \
    NUM
#define WHITESPACE \
    case ' ': case '\t':
#define NEWLINE \
    case '\r': case '\n':
#define WHITESPACE_NEWLINE \
    WHITESPACE \
    NEWLINE

#define COMMENT \
    case '#':

#define NAME \
    ALPHA_NUM \
    case '~': case '-': case '_': case '.':

// NOTE: This is not scientific.. based on a quick test of trying to rename a text file via Explorer on Windows 10
#define FILENAME \
    ALPHA_NUM \
    case '`': case '~': case '!': case '@': case '#': case '$': case '%': case '^': case '&': case '(': \
    case ')': case '-': case '_': case '=': case '+': case '[': case '{': case ']': case '}': case ';': \
    case '\'': case ',': case '.':
#define PATH_SEPARATOR \
    case '/':
#define PATH \
    PATH_SEPARATOR \
    FILENAME


typedef enum TokenType {
    TOK_UNKNOWN,
    TOK_ANIMATION,
    TOK_DIRSPRITE,
    TOK_FRAME,
    TOK_SPRITESHEET,
    TOK_SPRITE,
} TokenType;

typedef struct Token {
    TokenType type;
    StringView token;
} Token;

typedef struct Scanner {
    const char *fileName;
    size_t cursor;
    size_t length;
    const char *text;
} Scanner;

static bool DiscardChar(Scanner *scanner, char c)
{
    assert(scanner);
    assert(c);

    if (scanner->cursor < scanner->length && scanner->text[scanner->cursor] == c) {
        scanner->cursor++;
        return true;
    }
    return false;
}

static bool DiscardString(Scanner *scanner, const char *str)
{
    assert(scanner);
    assert(str);

    size_t idx = 0;
    while (scanner->cursor < scanner->length && scanner->text[scanner->cursor] == str[idx]) {
        idx++;
        scanner->cursor++;
    }

    // If string has more characters, we didn't match it all. Reset cursor and return false.
    if (str[idx]) {
        scanner->cursor -= idx;
        return false;
    }
    return true;
}

static char PeekChar(Scanner *scanner)
{
    assert(scanner);

    char c = 0;
    if (scanner->cursor < scanner->length) {
        c = scanner->text[scanner->cursor];
    }
    return c;
}


static bool ConsumePositiveInt(Scanner *scanner, int *value)
{
    assert(scanner);

    bool foundNum = false;
    int accum = 0;
    char c = PeekChar(scanner);
    while (c) {
        switch (c) {
            NUM {
                accum = accum * 10 + (int)(c - '0');
                if (accum < 0) {
                    // TODO: Better error info (line/column)
                    TraceLog(LOG_FATAL, "'%s': Integer overflow in ConsumeInt.\n", scanner->fileName);
                    exit(-1);
                }
                foundNum = true;
                scanner->cursor++;
                c = PeekChar(scanner);
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }

    if (value && foundNum) {
        *value = accum;
    }
    return foundNum;
}

static StringView ConsumeString_Alpha(Scanner *scanner)
{
    assert(scanner);

    StringView view = { 0 };
    view.text = scanner->text + scanner->cursor;

    char c = PeekChar(scanner);
    while (c) {
        switch (c) {
            ALPHA {
                view.length++;
                scanner->cursor++;
                c = PeekChar(scanner);
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }
    return view;
}

static StringView ConsumeString_Name(Scanner *scanner)
{
    assert(scanner);

    StringView view = { 0 };
    view.text = scanner->text + scanner->cursor;

    char c = PeekChar(scanner);
    while (c) {
        switch (c) {
            NAME {
                view.length++;
                scanner->cursor++;
                c = PeekChar(scanner);
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }

    return view;
}

static bool ConsumeString_Path(Scanner *scanner, char *buf, size_t bufLength)
{
    assert(scanner);

    bool foundPath = false;
    size_t bufCursor = 0;

    char c = PeekChar(scanner);
    while (c && bufCursor < bufLength) {
        switch (c) {
            PATH {
                buf[bufCursor] = c;
                bufCursor++;
                scanner->cursor++;
                c = PeekChar(scanner);
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }

    if (c && bufCursor == bufLength) {
        TraceLog(LOG_FATAL, "%s: Internal buffer is not big enough (max: %zu) to hold that path.\n", scanner->fileName, bufLength);
        assert(!"Internal path buffer not big enough");
        exit(-1);
    }
    return true;
}

static void DiscardWhitespace(Scanner *scanner)
{
    assert(scanner);

    while (scanner->cursor < scanner->length) {
        switch (scanner->text[scanner->cursor]) {
            WHITESPACE {
                scanner->cursor++;
                break;
            }
            default: {
                return;
            }
        }
    }
}

static void DiscardNewlines(Scanner *scanner)
{
    assert(scanner);

    while (scanner->cursor < scanner->length) {
        switch (scanner->text[scanner->cursor]) {
            NEWLINE {
                scanner->cursor++;
                break;
            }
            default: {
                return;
            }
        }
    }
}

static void DiscardComment(Scanner *scanner)
{
    assert(scanner);

    // Discard remainder of line
    while (scanner->cursor < scanner->length && scanner->text[scanner->cursor] != '\n') {
        scanner->cursor++;
    }
}

static void DiscardWhitespaceNewlinesComments(Scanner *scanner)
{
    assert(scanner);

    while (scanner->cursor < scanner->length) {
        switch (scanner->text[scanner->cursor]) {
            WHITESPACE_NEWLINE {
                scanner->cursor++;
                break;
            }
            COMMENT {
                DiscardComment(scanner);
                break;
            }
            default: {
                return;
            }
        }
    }
}

static TokenType GetIdentifierType(Scanner *scanner, StringView tok)
{
    assert(scanner);
    assert(tok.length);
    assert(tok.text);

    TokenType type = TOK_UNKNOWN;

    // TODO: Hash table
    if      (!strncmp(tok.text, "animation",   tok.length)) { type = TOK_ANIMATION;   }
    else if (!strncmp(tok.text, "frame",       tok.length)) { type = TOK_FRAME;       }
    else if (!strncmp(tok.text, "sprite",      tok.length)) { type = TOK_SPRITE;      }
    else if (!strncmp(tok.text, "spritesheet", tok.length)) { type = TOK_SPRITESHEET; }

    return type;
}

static Token ParseIdentifier(Scanner *scanner)
{
    assert(scanner);

    Token tok{};
    tok.token = ConsumeString_Alpha(scanner);
    tok.type = GetIdentifierType(scanner, tok.token);
    if (tok.type == TOK_UNKNOWN) {
        // TODO: Report better error info (line/column)
        TraceLog(LOG_ERROR, "Encountered unrecognized tokifer '%.*s' in file '%s'.\n", tok.token.length,
            tok.token.text, scanner->fileName);
    }
    return tok;
}

static bool ParseHeader(Scanner *scanner, Spritesheet *spritesheet)
{
    assert(scanner);
    assert(spritesheet);

#define USAGE "  Usage: spritesheet <frame_count> <animation_count> <sprite_count> <texture_path>\n" \
              "Example: spritesheet 8 1 1 resources/coin_gold.png\n"

    // frame count
    DiscardWhitespaceNewlinesComments(scanner);
    if (!ConsumePositiveInt(scanner, &spritesheet->frameCount)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame_count.\n" USAGE, scanner->fileName);
        return false;
    } else if (spritesheet->frameCount <= 0) {
        TraceLog(LOG_ERROR, "'%s': frame_count must be a positive, non-zero integer.\n" USAGE, scanner->fileName);
        return false;
    }

    // animation count
    DiscardWhitespaceNewlinesComments(scanner);
    if (!ConsumePositiveInt(scanner, &spritesheet->animationCount)) {
        TraceLog(LOG_ERROR, "'%s': Expected animation_count.\n" USAGE, scanner->fileName);
        return false;
    } else if (spritesheet->animationCount <= 0) {
        TraceLog(LOG_ERROR, "'%s': animation_count must be a positive, non-zero integer.\n" USAGE, scanner->fileName);
        return false;
    }

    // sprite count
    DiscardWhitespaceNewlinesComments(scanner);
    if (!ConsumePositiveInt(scanner, &spritesheet->spriteCount)) {
        TraceLog(LOG_ERROR, "'%s': Expected sprite_count.\n" USAGE, scanner->fileName);
        return false;
    } else if (spritesheet->spriteCount <= 0) {
        TraceLog(LOG_ERROR, "'%s': sprite_count must be a positive, non-zero integer.\n" USAGE, scanner->fileName);
        return false;
    }

    // texture path
    DiscardWhitespaceNewlinesComments(scanner);
    char texturePath[256] = { 0 };
    if (!ConsumeString_Path(scanner, texturePath, sizeof(texturePath) - 1)) {
        TraceLog(LOG_ERROR, "'%s': Expected texture_path. " USAGE "\n", scanner->fileName);
        return false;
    }

    //--------------------------------------------------------------------------------
    // Initialization, not parsing.. but it seems appropriate for it to live here?
    //--------------------------------------------------------------------------------
    // Allocate memory for frames
    spritesheet->frames = (SpriteFrame *)calloc(spritesheet->frameCount, sizeof(*spritesheet->frames));
    if (!spritesheet->frames) {
        TraceLog(LOG_ERROR, "'%s': Failed to allocate memory for %d frames in spritesheet.\n", scanner->fileName, spritesheet->frameCount);
        return false;
    }

    // Allocate memory for animations
    spritesheet->animations = (SpriteAnim *)calloc(spritesheet->animationCount, sizeof(*spritesheet->animations));
    if (!spritesheet->animations) {
        TraceLog(LOG_ERROR, "'%s': Failed to allocate memory for %d sprites in spritesheet.\n", scanner->fileName, spritesheet->animationCount);
        return false;
    }

    // Allocate memory for sprites
    spritesheet->sprites = (SpriteDef *)calloc(spritesheet->spriteCount, sizeof(*spritesheet->sprites));
    if (!spritesheet->sprites) {
        TraceLog(LOG_ERROR, "'%s': Failed to allocate memory for %d sprites in spritesheet.\n", scanner->fileName, spritesheet->spriteCount);
        return false;
    }

    // Load spritesheet texture
    spritesheet->texture = LoadTexture(texturePath);
    if (!spritesheet->texture.width) {
        TraceLog(LOG_ERROR, "'%s': Failed to load spritesheet texture [path: %s].\n", scanner->fileName, texturePath);
        return false;
    }
    //--------------------------------------------------------------------------------

    return true;
#undef USAGE
}

static bool ParseFrame(Scanner *scanner, SpriteFrame *frame)
{
    assert(scanner);
    assert(frame);

#define USAGE "  Usage: frame <name> <x> <y> <width> <height>\n" \
              "Example: frame player_melee_idle 0 0 54 94\n"

    // name
    DiscardWhitespaceNewlinesComments(scanner);
    frame->name = ConsumeString_Name(scanner);
    if (!frame->name.length) {
        TraceLog(LOG_ERROR, "'%s': Expected name.\n" USAGE, scanner->fileName);
        return false;
    }

    // x
    DiscardWhitespaceNewlinesComments(scanner);
    if (!ConsumePositiveInt(scanner, &frame->x)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame x (left).\n" USAGE, scanner->fileName);
        return false;
    }

    // y
    DiscardWhitespaceNewlinesComments(scanner);
    if (!ConsumePositiveInt(scanner, &frame->y)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame y (top).\n" USAGE, scanner->fileName);
        return false;
    }

    // width
    DiscardWhitespaceNewlinesComments(scanner);
    if (!ConsumePositiveInt(scanner, &frame->width)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame width.\n" USAGE, scanner->fileName);
        return false;
    } else if (frame->width <= 0) {
        TraceLog(LOG_ERROR, "'%s': width must be a positive, non-zero integer.\n" USAGE, scanner->fileName);
        return false;
    }

    // height
    DiscardWhitespaceNewlinesComments(scanner);
    if (!ConsumePositiveInt(scanner, &frame->height)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame height.\n" USAGE, scanner->fileName);
        return false;
    } else if (frame->height <= 0) {
        TraceLog(LOG_ERROR, "'%s': height must be a positive, non-zero integer.\n" USAGE, scanner->fileName);
        return false;
    }

    return true;
#undef USAGE
}

static bool ParseAnimation(Scanner *scanner, SpriteAnim *animation)
{
    assert(scanner);
    assert(animation);

#define USAGE "  Usage: animation <name> <frame0> [frame1] ... [frame15]\n" \
              "Example: animation coin_spin 0 1 2 3 4 5 6 7\n"

    // name
    DiscardWhitespaceNewlinesComments(scanner);
    animation->name = ConsumeString_Name(scanner);
    if (!animation->name.length) {
        TraceLog(LOG_ERROR, "'%s': Expected name.\n" USAGE, scanner->fileName);
        return false;
    }

    // frames
    for (int i = 0; i < SPRITEANIM_MAX_FRAMES; i++) {
        DiscardWhitespaceNewlinesComments(scanner);
        if (ConsumePositiveInt(scanner, &animation->frames[i])) {
            animation->frameCount++;
        } else {
            animation->frames[i] = -1;
        }
    }

    if (!animation->frameCount) {
        TraceLog(LOG_ERROR, "'%s': Expected at least one frame index.\n" USAGE, scanner->fileName);
        return false;
    }

    return true;
#undef USAGE
}

static bool ParseSprite(Scanner *scanner, SpriteDef *sprite)
{
    assert(scanner);
    assert(sprite);

#define USAGE "  Usage: sprite <name> <anim_n> <anim_e> <anim_s> <anim_w> <anim_ne> <anim_se> <anim_sw> <anim_nw>\n" \
              "Example: sprite player_sword 10 11 12 13 14 15 16 17\n"

    // name
    DiscardWhitespaceNewlinesComments(scanner);
    sprite->name = ConsumeString_Name(scanner);
    if (!sprite->name.length) {
        TraceLog(LOG_ERROR, "'%s': Expected name.\n" USAGE, scanner->fileName);
        return false;
    }

    // directional animations
    for (int i = 0; i < Direction_Count; i++) {
        DiscardWhitespaceNewlinesComments(scanner);
        if (DiscardChar(scanner, '-')) {
            sprite->animations[i] = -1;
        } else if (!ConsumePositiveInt(scanner, &sprite->animations[i])) {
            TraceLog(LOG_ERROR, "'%s': Expected an animation index for each of the %d directions.\n" USAGE, scanner->fileName, (int)Direction_Count);
            return false;
        }
    }

    return true;
#undef USAGE
}

static bool ParseSpritesheet(Scanner *scanner, Spritesheet *spritesheet)
{
    int framesParsed = 0;
    int animationsParsed = 0;
    int spritesParsed = 0;
    int dirSpritesParsed = 0;

    DiscardWhitespaceNewlinesComments(scanner);
    char c = PeekChar(scanner);
    while (c) {
        switch (c) {
            ALPHA {
                Token tok = ParseIdentifier(scanner);
                switch (tok.type) {
                    case TOK_SPRITESHEET: {
                        if (!ParseHeader(scanner, spritesheet)) {
                            return false;
                        }
                        break;
                    }
                    case TOK_FRAME: {
                        if (!spritesheet->frameCount) {
                            TraceLog(LOG_ERROR, "'%s': Error: Encountered 'spriteframe' before 'header'.\n", scanner->fileName);
                            return false;
                        }
                        if (framesParsed >= spritesheet->frameCount) {
                            TraceLog(LOG_ERROR, "'%s': Error: Encountered 'spriteframe', but [%d of %d] frames have already been added to the spritesheet.\n", scanner->fileName, framesParsed, spritesheet->frameCount);
                            return false;
                        }
                        SpriteFrame *frame = &spritesheet->frames[framesParsed];
                        if (!ParseFrame(scanner, frame)) {
                            return false;
                        }
                        framesParsed++;
                        break;
                    }
                    case TOK_ANIMATION: {
                        if (!spritesheet->animationCount) {
                            TraceLog(LOG_ERROR, "'%s': Error: Encountered 'animation' before 'header'.\n", scanner->fileName);
                            return false;
                        }
                        if (animationsParsed >= spritesheet->animationCount) {
                            TraceLog(LOG_ERROR, "'%s': Error: Encountered 'animation', but [%d of %d] animations have already been added to the spritesheet.\n", scanner->fileName, animationsParsed, spritesheet->animationCount);
                            return false;
                        }
                        SpriteAnim *animation = &spritesheet->animations[animationsParsed];
                        if (!ParseAnimation(scanner, animation)) {
                            return false;
                        }
                        animationsParsed++;
                        break;
                    }
                    case TOK_SPRITE: {
                        if (!spritesheet->spriteCount) {
                            TraceLog(LOG_ERROR, "'%s': Error: Encountered 'sprite' before 'header'.\n", scanner->fileName);
                            return false;
                        }
                        if (spritesParsed >= spritesheet->spriteCount) {
                            TraceLog(LOG_ERROR, "'%s': Error: Encountered 'sprite', but [%d of %d] sprites have already been added to the spritesheet.\n", scanner->fileName, spritesParsed, spritesheet->spriteCount);
                            return false;
                        }
                        SpriteDef *sprite = &spritesheet->sprites[spritesParsed];
                        sprite->spritesheet = spritesheet;
                        if (!ParseSprite(scanner, sprite)) {
                            return false;
                        }
                        spritesParsed++;
                        break;
                    }
                    default: {
                        // TODO: Better error handling
                        TraceLog(LOG_ERROR, "'%s': Error: Unrecognized token '%.*s'\n.", scanner->fileName, tok.token.length, tok.token.text);
                        assert(!"Unrecognized token");
                        break;
                    }
                }
                break;
            }
            default: {
                // TODO: Better error handling
                TraceLog(LOG_ERROR, "'%s': Error: Unexpected character '%c'\n.", scanner->fileName, c);
                assert(!"Unexpected character");
                break;
            }
        }
        DiscardWhitespaceNewlinesComments(scanner);
        c = PeekChar(scanner);
    }

    if (framesParsed < spritesheet->frameCount) {
        TraceLog(LOG_ERROR, "'%s': Error: Expected to find %d frames, only found %d.\n", scanner->fileName, spritesheet->frameCount, framesParsed);
        return false;
    } else if (animationsParsed < spritesheet->animationCount) {
        TraceLog(LOG_ERROR, "'%s': Error: Expected to find %d animations, only found %d.\n", scanner->fileName, spritesheet->animationCount, animationsParsed);
        return false;
    } else if (spritesParsed < spritesheet->spriteCount) {
        TraceLog(LOG_ERROR, "'%s': Error: Expected to find %d sprites, only found %d.\n", scanner->fileName, spritesheet->spriteCount, spritesParsed);
        return false;
    }

    return true;
}

void spritesheet_init(Spritesheet *spritesheet, const char *fileName)
{
    unsigned int dataLength = 0;
    char *data = (char *)LoadFileData(fileName, &dataLength);
    if (!data) {
        return;
    }
    spritesheet->buf = data;
    spritesheet->bufLength = dataLength;

    Scanner scanner = { 0 };
    scanner.fileName = fileName;
    scanner.text = spritesheet->buf;
    scanner.length = spritesheet->bufLength;
    if (!ParseSpritesheet(&scanner, spritesheet)) {
        spritesheet_free(spritesheet);
    }
}

const SpriteDef *spritesheet_find_sprite(const Spritesheet *spritesheet, const char *name)
{
    // TODO: Hash table if the # of sprites per sheet grows to > 16.. or make SpriteID / sprite catalog?
    for (int i = 0; i < spritesheet->spriteCount; i++) {
        const SpriteDef *sprite = &spritesheet->sprites[i];
        if (!strncmp(sprite->name.text, name, sprite->name.length)) {
            return sprite;
        }
    }
    return 0;
}

void spritesheet_free(Spritesheet *spritesheet)
{
    free(spritesheet->frames);
    free(spritesheet->animations);
    free(spritesheet->sprites);
    UnloadTexture(spritesheet->texture);
    UnloadFileData((unsigned char *)spritesheet->buf);
    memset(spritesheet, 0, sizeof(*spritesheet));
}