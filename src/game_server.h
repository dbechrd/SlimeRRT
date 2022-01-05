#pragma once
#include "args.h"
#include "error.h"
#include "net_server.h"
#include "world.h"
#include <thread>

struct GameServer {
    GameServer(const Args &args) : args(args) {};
    ErrorType Run();

private:
    static const char *LOG_SRC;
    const Args &args;
    NetServer  netServer {};
};