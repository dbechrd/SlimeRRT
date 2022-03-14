#pragma once
#include "args.h"
#include "error.h"
#include "net_server.h"
#include "world.h"
#include <thread>

struct GameServer {
    static std::thread &StartThread(const Args &args);
    ErrorType Run(const Args &args);

private:
    static const char *LOG_SRC;
    NetServer netServer {};
};