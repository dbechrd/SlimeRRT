#pragma once
#include "error.h"
#include "helpers.h"
#include <atomic>

struct Args {
    bool              standalone {};
    const char       *host       { SV_SINGLEPLAYER_HOST };
    unsigned short    port       { SV_SINGLEPLAYER_PORT };
    const char       *user       { SV_SINGLEPLAYER_USER };
    const char       *pass       { SV_SINGLEPLAYER_PASS };
    std::atomic<bool> serverQuit { false };

    ErrorType Parse(int argc, char *argv[]);
};
