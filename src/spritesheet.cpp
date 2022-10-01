#include "spritesheet.h"
#include "body.h"
#include "helpers.h"
#include "raylib/raylib.h"
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

struct Token {
    enum class Type {
        Unknown,
        Animation,
        Frame,
        Spritesheet,
        Sprite,
    };

    Token::Type type;
    StringView token;
};

struct Scanner {
    bool DiscardChar(char c);
    bool DiscardString(const char *str);
    char PeekChar() const;
    ErrorType ConsumePositiveInt(int *value);
    StringView ConsumeString_Alpha();
    StringView ConsumeString_Name();
    ErrorType ConsumeString_Path(char *buf, size_t bufLength);
    void DiscardWhitespace();
    void DiscardNewlines();
    void DiscardComment();
    void DiscardWhitespaceNewlinesComments();
    Token::Type GetIdentifierType(StringView tok);
    ErrorType ParseIdentifier(Token *token);
    ErrorType ParseHeader(Spritesheet &spritesheet);
    ErrorType ParseFrame(SpriteFrame &frame);
    ErrorType ParseAnimation(SpriteAnim &animation);
    ErrorType ParseSprite(SpriteDef &sprite);
    ErrorType ParseSpritesheet(Spritesheet &spritesheet);

public:
    const char *LOG_SRC = "Scanner";
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

ErrorType Scanner::ConsumePositiveInt(int *value)
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
                    E_CHECKMSG(ErrorType::Overflow, "'%s': Integer overflow in ConsumeInt.\n", fileName);
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
    if (!foundNum) {
        return ErrorType::NotFound;
    }
    return ErrorType::Success;
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

ErrorType Scanner::ConsumeString_Path(char *buf, size_t bufLength)
{
    size_t bufCursor = 0;

    bool foundPath = false;
    char c = PeekChar();
    while (c && bufCursor < bufLength) {
        switch (c) {
            PATH {
                buf[bufCursor] = c;
                bufCursor++;
                foundPath = true;
                cursor++;
                c = PeekChar();
                break;
            }
            default: {
                c = 0;  // break outer loop
            }
        }
    }

    if (!foundPath) {
        E_CHECKMSG(ErrorType::FileReadFailed, "Expected path string, found '%c' instead.", c);
    } else if (c && bufCursor == bufLength) {
        E_CHECKMSG(ErrorType::FileReadFailed, "%s: Internal buffer is not big enough (max: %zu) to hold that path.\n", fileName, bufLength);
    }
    return ErrorType::Success;
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

Token::Type Scanner::GetIdentifierType(StringView tok)
{
    assert(tok.length);
    assert(tok.text);

    Token::Type type = Token::Type::Unknown;

    // TODO: Hash table
    if      (!strncmp(tok.text, "animation",   tok.length)) { type = Token::Type::Animation;   }
    else if (!strncmp(tok.text, "frame",       tok.length)) { type = Token::Type::Frame;       }
    else if (!strncmp(tok.text, "sprite",      tok.length)) { type = Token::Type::Sprite;      }
    else if (!strncmp(tok.text, "spritesheet", tok.length)) { type = Token::Type::Spritesheet; }

    return type;
}

ErrorType Scanner::ParseIdentifier(Token *token)
{
    Token tok{};
    tok.token = ConsumeString_Alpha();
    tok.type = GetIdentifierType(tok.token);
    if (tok.type == Token::Type::Unknown) {
        // TODO: Report better error info (line/column)
        E_CHECKMSG(ErrorType::FileReadFailed, "Encountered unrecognized token '%.*s' in file '%s'.\n", tok.token.length,
            tok.token.text, fileName);
    }
    if (token) *token = tok;
    return ErrorType::Success;
}

ErrorType Scanner::ParseHeader(Spritesheet &spritesheet)
{
#define USAGE "  Usage: spritesheet <frame_count> <animation_count> <sprite_count> <texture_path>\n" \
              "Example: spritesheet 8 1 1 data/texture/item/coin_copper.png\n"

    int frameCount = 0;
    int animationCount = 0;
    int spriteCount = 0;

    // frame count
    DiscardWhitespaceNewlinesComments();
    if (ConsumePositiveInt(&frameCount) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected frame_count.\n" USAGE, fileName);
    } else if (frameCount <= 0) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': frame_count must be a positive, non-zero integer.\n" USAGE, fileName);
    }

    // animation count
    DiscardWhitespaceNewlinesComments();
    if (ConsumePositiveInt(&animationCount) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected animation_count.\n" USAGE, fileName);
    } else if (animationCount < 0) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': animation_count must be a positive integer or zero.\n" USAGE, fileName);
    }

    // sprite count
    DiscardWhitespaceNewlinesComments();
    if (ConsumePositiveInt(&spriteCount) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected sprite_count.\n" USAGE, fileName);
    } else if (spriteCount <= 0) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': sprite_count must be a positive, non-zero integer.\n" USAGE, fileName);
    }

    // texture path
    DiscardWhitespaceNewlinesComments();
    char texturePath[256] = {};
    if (ConsumeString_Path(texturePath, sizeof(texturePath) - 1) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected texture_path. " USAGE "\n", fileName);
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
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Failed to load spritesheet texture [path: %s].\n", fileName, texturePath);
    }
    //--------------------------------------------------------------------------------

    return ErrorType::Success;
#undef USAGE
}

ErrorType Scanner::ParseFrame(SpriteFrame &frame)
{
#define USAGE "  Usage: frame <name> <x> <y> <width> <height>\n" \
              "Example: frame player_melee_idle 0 0 54 94\n"

    // name
    DiscardWhitespaceNewlinesComments();
    frame.name = ConsumeString_Name();
    if (!frame.name.length) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected name.\n" USAGE, fileName);
    }

    // x
    DiscardWhitespaceNewlinesComments();
    if (ConsumePositiveInt(&frame.x) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected frame x (left).\n" USAGE, fileName);
    }

    // y
    DiscardWhitespaceNewlinesComments();
    if (ConsumePositiveInt(&frame.y) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected frame y (top).\n" USAGE, fileName);
    }

    // width
    DiscardWhitespaceNewlinesComments();
    if (ConsumePositiveInt(&frame.width) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected frame width.\n" USAGE, fileName);
    } else if (frame.width <= 0) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': width must be a positive, non-zero integer.\n" USAGE, fileName);
    }

    // height
    DiscardWhitespaceNewlinesComments();
    if (ConsumePositiveInt(&frame.height) != ErrorType::Success) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected frame height.\n" USAGE, fileName);
    } else if (frame.height <= 0) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': height must be a positive, non-zero integer.\n" USAGE, fileName);
    }

    return ErrorType::Success;
#undef USAGE
}

ErrorType Scanner::ParseAnimation(SpriteAnim &animation)
{
#define USAGE "  Usage: animation <name> <frame0> [frame1] ... [frame15]\n" \
              "Example: animation coin_spin 0 1 2 3 4 5 6 7\n"

    // name
    DiscardWhitespaceNewlinesComments();
    animation.name = ConsumeString_Name();
    if (!animation.name.length) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected name.\n" USAGE, fileName);
    }

    // frames
    for (int i = 0; i < SPRITEANIM_MAX_FRAMES; i++) {
        DiscardWhitespaceNewlinesComments();
        ErrorType err = ConsumePositiveInt(&animation.frames[i]);
        switch (err) {
            case ErrorType::Success:
                animation.frameCount++;
                break;
            case ErrorType::NotFound:
                animation.frames[i] = -1;
                break;
            default:
                E_CHECKMSG(err, "Failed to parse spriteanim frame");
        }
    }

    if (!animation.frameCount) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected at least one frame index.\n" USAGE, fileName);
    }

    return ErrorType::Success;
#undef USAGE
}

ErrorType Scanner::ParseSprite(SpriteDef &sprite)
{
#define USAGE "  Usage: sprite <name> <anim_n> <anim_e> <anim_s> <anim_w> <anim_ne> <anim_se> <anim_sw> <anim_nw>\n" \
              "Example: sprite player_sword 10 11 12 13 14 15 16 17\n"

    // name
    DiscardWhitespaceNewlinesComments();
    sprite.name = ConsumeString_Name();
    if (!sprite.name.length) {
        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected name.\n" USAGE, fileName);
    }

    // directional animations
    for (int i = 0; i < (int)Direction::Count; i++) {
        DiscardWhitespaceNewlinesComments();
        if (DiscardChar('-')) {
            sprite.animations[i] = -1;
        } else if (ConsumePositiveInt(&sprite.animations[i]) != ErrorType::Success) {
            E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Expected an animation index for each of the %d directions.\n" USAGE, fileName, (int)Direction::Count);
        }
    }

    return ErrorType::Success;
#undef USAGE
}

ErrorType Scanner::ParseSpritesheet(Spritesheet &spritesheet)
{
    int framesParsed = 0;
    int animationsParsed = 0;
    int spritesParsed = 0;

    DiscardWhitespaceNewlinesComments();
    char c = PeekChar();
    while (c) {
        switch (c) {
            ALPHA {
                Token tok{};
                E_CHECKMSG(ParseIdentifier(&tok), "Failed to parse token");
                switch (tok.type) {
                    case Token::Type::Spritesheet: {
                        E_CHECKMSG(ParseHeader(spritesheet), "Failed to parse header");
                        break;
                    }
                    case Token::Type::Frame: {
                        SpriteFrame &frame = spritesheet.frames.emplace_back();
                        E_CHECKMSG(ParseFrame(frame), "Failed to parse frame");
                        framesParsed++;
                        break;
                    }
                    case Token::Type::Animation: {
                        SpriteAnim &animation = spritesheet.animations.emplace_back();
                        E_CHECKMSG(ParseAnimation(animation), "Failed to parse animation");
                        animationsParsed++;
                        break;
                    }
                    case Token::Type::Sprite: {
                        SpriteDef &sprite = spritesheet.sprites.emplace_back(&spritesheet);
                        E_CHECKMSG(ParseSprite(sprite), "Failed to parse sprite");
                        spritesParsed++;
                        break;
                    }
                    default: {
                        E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Error: Unrecognized token '%.*s'\n.", fileName, tok.token.length, tok.token.text);
                        break;
                    }
                }
                break;
            }
            default: {
                E_CHECKMSG(ErrorType::FileReadFailed, "'%s': Error: Unexpected character '%c'\n.", fileName, c);
                break;
            }
        }
        DiscardWhitespaceNewlinesComments();
        c = PeekChar();
    }

    return ErrorType::Success;
}

SpriteDef::SpriteDef(const Spritesheet *spritesheet)
{
    this->spritesheet = spritesheet;
}

ErrorType Spritesheet::LoadFromFile(const char *filename)
{
    buf = (char *)LoadFileData(filename, &bufLength);
    if (!buf) {
        E_CHECKMSG(ErrorType::FileReadFailed, "Failed to read file %s", filename);
    }

    Scanner scanner = {};
    scanner.fileName = filename;
    scanner.text = buf;
    scanner.length = bufLength;

    return scanner.ParseSpritesheet(*this);
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