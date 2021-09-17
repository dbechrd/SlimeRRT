#pragma once
#include "args.h"
#include "error.h"
#include "net_server.h"
#include "world.h"
#include <thread>

struct GameServer {
    GameServer(Args args);
    ErrorType Run();

private:
    static const char *LOG_SRC;
    Args      args;
    World *   world     {};
    NetServer netServer {};
};