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


enum class TokenType {
    Unknown,
    Animation,
    Frame,
    Spritesheet,
    Sprite,
};

struct Token {
    TokenType type;
    StringView token;
};

struct Scanner {
    bool DiscardChar(char c);
    bool DiscardString(const char *str);
    char PeekChar() const;
    bool ConsumePositiveInt(int *value);
    StringView ConsumeString_Alpha();
    StringView ConsumeString_Name();
    bool ConsumeString_Path(char *buf, size_t bufLength);
    void DiscardWhitespace();
    void DiscardNewlines();
    void DiscardComment();
    void DiscardWhitespaceNewlinesComments();
    TokenType GetIdentifierType(StringView tok);
    Token ParseIdentifier();
    bool ParseHeader(Spritesheet &spritesheet);
    bool ParseFrame(SpriteFrame &frame);
    bool ParseAnimation(SpriteAnim &animation);
    bool ParseSprite(SpriteDef &sprite);
    bool ParseSpritesheet(Spritesheet &spritesheet);

public:
    const char *fileName;
    size_t cursor;
    size_t length;
    const char *text;
};

bool Scanner::DiscardChar(char c)
{
    assert(c);

    if (cursor < length && text[cursor] == c) {
        cursor++;
        return true;
    }
    return false;
}

bool Scanner::DiscardString(const char *str)
{
    assert(str);

    size_t idx = 0;
    while (cursor < length && text[cursor] == str[idx]) {
        idx++;
        cursor++;
    }

    // If string has more characters, we didn't match it all. Reset cursor and return false.
    if (str[idx]) {
        cursor -= idx;
        return false;
    }
    return true;
}

char Scanner::PeekChar() const
{
    char c = 0;
    if (cursor < length) {
        c = text[cursor];
    }
    return c;
}

bool Scanner::ConsumePositiveInt(int *value)
{
    bool foundNum = false;
    int accum = 0;
    char c = PeekChar();
    while (c) {
        switch (c) {
            NUM {
                accum = accum * 10 + (int)(c - '0');
                if (accum < 0) {
                    // TODO: Better error info (line/column)
                    TraceLog(LOG_FATAL, "'%s': Integer overflow in ConsumeInt.\n", fileName);
                    exit(-1);
                }
                foundNum = true;
                cursor++;
                c = PeekChar();
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

StringView Scanner::ConsumeString_Alpha()
{
    StringView view = {};
    view.text = text + cursor;

    char c = PeekChar();
    while (c) {
        switch (c) {
            ALPHA {
                view.length++;
                cursor++;
                c = PeekChar();
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }
    return view;
}

StringView Scanner::ConsumeString_Name()
{
    StringView view = {};
    view.text = text + cursor;

    char c = PeekChar();
    while (c) {
        switch (c) {
            NAME {
                view.length++;
                cursor++;
                c = PeekChar();
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }

    return view;
}

bool Scanner::ConsumeString_Path(char *buf, size_t bufLength)
{
    size_t bufCursor = 0;

    char c = PeekChar();
    while (c && bufCursor < bufLength) {
        switch (c) {
            PATH {
                buf[bufCursor] = c;
                bufCursor++;
                cursor++;
                c = PeekChar();
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }

    if (c && bufCursor == bufLength) {
        TraceLog(LOG_FATAL, "%s: Internal buffer is not big enough (max: %zu) to hold that path.\n", fileName, bufLength);
        assert(!"Internal path buffer not big enough");
        exit(-1);
    }
    return true;
}

void Scanner::DiscardWhitespace()
{
    while (cursor < length) {
        switch (text[cursor]) {
            WHITESPACE {
                cursor++;
                break;
            }
            default: {
                return;
            }
        }
    }
}

void Scanner::DiscardNewlines()
{
    while (cursor < length) {
        switch (text[cursor]) {
            NEWLINE {
                cursor++;
                break;
            }
            default: {
                return;
            }
        }
    }
}

void Scanner::DiscardComment()
{
    // Discard remainder of line
    while (cursor < length && text[cursor] != '\n') {
        cursor++;
    }
}

void Scanner::DiscardWhitespaceNewlinesComments()
{
    while (cursor < length) {
        switch (text[cursor]) {
            WHITESPACE_NEWLINE {
                cursor++;
                break;
            }
            COMMENT {
                DiscardComment();
                break;
            }
            default: {
                return;
            }
        }
    }
}

TokenType Scanner::GetIdentifierType(StringView tok)
{
    assert(tok.length);
    assert(tok.text);

    TokenType type = TokenType::Unknown;

    // TODO: Hash table
    if      (!strncmp(tok.text, "animation",   tok.length)) { type = TokenType::Animation;   }
    else if (!strncmp(tok.text, "frame",       tok.length)) { type = TokenType::Frame;       }
    else if (!strncmp(tok.text, "sprite",      tok.length)) { type = TokenType::Sprite;      }
    else if (!strncmp(tok.text, "spritesheet", tok.length)) { type = TokenType::Spritesheet; }

    return type;
}

Token Scanner::ParseIdentifier()
{
    Token tok{};
    tok.token = ConsumeString_Alpha();
    tok.type = GetIdentifierType(tok.token);
    if (tok.type == TokenType::Unknown) {
        // TODO: Report better error info (line/column)
        TraceLog(LOG_ERROR, "Encountered unrecognized tokifer '%.*s' in file '%s'.\n", tok.token.length,
            tok.token.text, fileName);
    }
    return tok;
}

bool Scanner::ParseHeader(Spritesheet &spritesheet)
{
#define USAGE "  Usage: spritesheet <frame_count> <animation_count> <sprite_count> <texture_path>\n" \
              "Example: spritesheet 8 1 1 resources/coin_gold.png\n"

    int frameCount = 0;
    int animationCount = 0;
    int spriteCount = 0;

    // frame count
    DiscardWhitespaceNewlinesComments();
    if (!ConsumePositiveInt(&frameCount)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame_count.\n" USAGE, fileName);
        return false;
    } else if (frameCount <= 0) {
        TraceLog(LOG_ERROR, "'%s': frame_count must be a positive, non-zero integer.\n" USAGE, fileName);
        return false;
    }

    // animation count
    DiscardWhitespaceNewlinesComments();
    if (!ConsumePositiveInt(&animationCount)) {
        TraceLog(LOG_ERROR, "'%s': Expected animation_count.\n" USAGE, fileName);
        return false;
    } else if (animationCount <= 0) {
        TraceLog(LOG_ERROR, "'%s': animation_count must be a positive, non-zero integer.\n" USAGE, fileName);
        return false;
    }

    // sprite count
    DiscardWhitespaceNewlinesComments();
    if (!ConsumePositiveInt(&spriteCount)) {
        TraceLog(LOG_ERROR, "'%s': Expected sprite_count.\n" USAGE, fileName);
        return false;
    } else if (spriteCount <= 0) {
        TraceLog(LOG_ERROR, "'%s': sprite_count must be a positive, non-zero integer.\n" USAGE, fileName);
        return false;
    }

    // texture path
    DiscardWhitespaceNewlinesComments();
    char texturePath[256] = {};
    if (!ConsumeString_Path(texturePath, sizeof(texturePath) - 1)) {
        TraceLog(LOG_ERROR, "'%s': Expected texture_path. " USAGE "\n", fileName);
        return false;
    }

    //--------------------------------------------------------------------------------
    // Initialization, not parsing.. but it seems appropriate for it to live here?
    //--------------------------------------------------------------------------------
    // Allocate memory for frames
    // TODO: Handle bad_alloc?
    spritesheet.frames.reserve(frameCount);

    // Allocate memory for animations
    // TODO: Handle bad_alloc?
    spritesheet.animations.reserve(animationCount);

    // Allocate memory for sprites
    // TODO: Handle bad_alloc?
    spritesheet.sprites.reserve(spriteCount);

    // Load spritesheet texture
    spritesheet.texture = LoadTexture(texturePath);
    if (!spritesheet.texture.width) {
        TraceLog(LOG_ERROR, "'%s': Failed to load spritesheet texture [path: %s].\n", fileName, texturePath);
        return false;
    }
    //--------------------------------------------------------------------------------

    return true;
#undef USAGE
}

bool Scanner::ParseFrame(SpriteFrame &frame)
{
#define USAGE "  Usage: frame <name> <x> <y> <width> <height>\n" \
              "Example: frame player_melee_idle 0 0 54 94\n"

    // name
    DiscardWhitespaceNewlinesComments();
    frame.name = ConsumeString_Name();
    if (!frame.name.length) {
        TraceLog(LOG_ERROR, "'%s': Expected name.\n" USAGE, fileName);
        return false;
    }

    // x
    DiscardWhitespaceNewlinesComments();
    if (!ConsumePositiveInt(&frame.x)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame x (left).\n" USAGE, fileName);
        return false;
    }

    // y
    DiscardWhitespaceNewlinesComments();
    if (!ConsumePositiveInt(&frame.y)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame y (top).\n" USAGE, fileName);
        return false;
    }

    // width
    DiscardWhitespaceNewlinesComments();
    if (!ConsumePositiveInt(&frame.width)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame width.\n" USAGE, fileName);
        return false;
    } else if (frame.width <= 0) {
        TraceLog(LOG_ERROR, "'%s': width must be a positive, non-zero integer.\n" USAGE, fileName);
        return false;
    }

    // height
    DiscardWhitespaceNewlinesComments();
    if (!ConsumePositiveInt(&frame.height)) {
        TraceLog(LOG_ERROR, "'%s': Expected frame height.\n" USAGE, fileName);
        return false;
    } else if (frame.height <= 0) {
        TraceLog(LOG_ERROR, "'%s': height must be a positive, non-zero integer.\n" USAGE, fileName);
        return false;
    }

    return true;
#undef USAGE
}

bool Scanner::ParseAnimation(SpriteAnim &animation)
{
#define USAGE "  Usage: animation <name> <frame0> [frame1] ... [frame15]\n" \
              "Example: animation coin_spin 0 1 2 3 4 5 6 7\n"

    // name
    DiscardWhitespaceNewlinesComments();
    animation.name = ConsumeString_Name();
    if (!animation.name.length) {
        TraceLog(LOG_ERROR, "'%s': Expected name.\n" USAGE, fileName);
        return false;
    }

    // frames
    for (int i = 0; i < SPRITEANIM_MAX_FRAMES; i++) {
        DiscardWhitespaceNewlinesComments();
        if (ConsumePositiveInt(&animation.frames[i])) {
            animation.frameCount++;
        } else {
            animation.frames[i] = -1;
        }
    }

    if (!animation.frameCount) {
        TraceLog(LOG_ERROR, "'%s': Expected at least one frame index.\n" USAGE, fileName);
        return false;
    }

    return true;
#undef USAGE
}

bool Scanner::ParseSprite(SpriteDef &sprite)
{
#define USAGE "  Usage: sprite <name> <anim_n> <anim_e> <anim_s> <anim_w> <anim_ne> <anim_se> <anim_sw> <anim_nw>\n" \
              "Example: sprite player_sword 10 11 12 13 14 15 16 17\n"

    // name
    DiscardWhitespaceNewlinesComments();
    sprite.name = ConsumeString_Name();
    if (!sprite.name.length) {
        TraceLog(LOG_ERROR, "'%s': Expected name.\n" USAGE, fileName);
        return false;
    }

    // directional animations
    for (int i = 0; i < (int)Direction::Count; i++) {
        DiscardWhitespaceNewlinesComments();
        if (DiscardChar('-')) {
            sprite.animations[i] = -1;
        } else if (!ConsumePositiveInt(&sprite.animations[i])) {
            TraceLog(LOG_ERROR, "'%s': Expected an animation index for each of the %d directions.\n" USAGE, fileName, (int)Direction::Count);
            return false;
        }
    }

    return true;
#undef USAGE
}

bool Scanner::ParseSpritesheet(Spritesheet &spritesheet)
{
    int framesParsed = 0;
    int animationsParsed = 0;
    int spritesParsed = 0;

    DiscardWhitespaceNewlinesComments();
    char c = PeekChar();
    while (c) {
        switch (c) {
            ALPHA {
                Token tok = ParseIdentifier();
                switch (tok.type) {
                    case TokenType::Spritesheet: {
                        if (!ParseHeader(spritesheet)) {
                            return false;
                        }
                        break;
                    }
                    case TokenType::Frame: {
                        SpriteFrame &frame = spritesheet.frames.emplace_back();
                        if (!ParseFrame(frame)) {
                            return false;
                        }
                        framesParsed++;
                        break;
                    }
                    case TokenType::Animation: {
                        SpriteAnim &animation = spritesheet.animations.emplace_back();
                        if (!ParseAnimation(animation)) {
                            return false;
                        }
                        animationsParsed++;
                        break;
                    }
                    case TokenType::Sprite: {
                        SpriteDef &sprite = spritesheet.sprites.emplace_back(&spritesheet);
                        if (!ParseSprite(sprite)) {
                            return false;
                        }
                        spritesParsed++;
                        break;
                    }
                    default: {
                        // TODO: Better error handling
                        TraceLog(LOG_ERROR, "'%s': Error: Unrecognized token '%.*s'\n.", fileName, tok.token.length, tok.token.text);
                        assert(!"Unrecognized token");
                        break;
                    }
                }
                break;
            }
            default: {
                // TODO: Better error handling
                TraceLog(LOG_ERROR, "'%s': Error: Unexpected character '%c'\n.", fileName, c);
                assert(!"Unexpected character");
                break;
            }
        }
        DiscardWhitespaceNewlinesComments();
        c = PeekChar();
    }

    return true;
}

SpriteDef::SpriteDef(const Spritesheet *spritesheet)
{
    this->spritesheet = spritesheet;
}

bool Spritesheet::LoadFromFile(const char *fileName)
{
    buf = (char *)LoadFileData(fileName, &bufLength);
    if (!buf) {
        return false;
    }

    Scanner scanner = {};
    scanner.fileName = fileName;
    scanner.text = buf;
    scanner.length = bufLength;

    bool success = scanner.ParseSpritesheet(*this);
    return success;
}

Spritesheet::~Spritesheet()
{
    frames.clear();
    animations.clear();
    sprites.clear();
    UnloadTexture(texture);
    UnloadFileData((unsigned char *)buf);
}

const SpriteDef *Spritesheet::FindSprite(const char *name) const
{
    // TODO: Hash table if the # of sprites per sheet grows to > 16.. or make SpriteID / sprite catalog?
    for (const SpriteDef &sprite : sprites) {
        if (!strncmp(sprite.name.text, name, sprite.name.length)) {
            return &sprite;
        }
    }

    // TODO: Return reference to some ugly placeholder sprite instead
    return 0;
}