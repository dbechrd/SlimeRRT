#pragma once
#include <stdint.h>

typedef struct StringView {
    size_t length;              // length of string (without nil terminator, these strings are views into a larger buffer)
    const unsigned char *text;  // pointer into someone else's buffer, do not attempt to free
} StringView;