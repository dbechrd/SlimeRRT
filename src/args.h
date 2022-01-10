#pragma once
#include "helpers.h"
#include <atomic>

struct Args {
    bool              standalone {};
    const char       *host       { "slime.theprogrammingjunkie.com" }; // SV_SINGLEPLAYER_HOST };
    unsigned short    port       { SV_DEFAULT_PORT }; // SV_SINGLEPLAYER_PORT };
    const char       *user       { SV_SINGLEPLAYER_USER };
    const char       *pass       { SV_SINGLEPLAYER_PASS };
    std::atomic<bool> exiting    { false };

    Args(int argc, char *argv[]);
};