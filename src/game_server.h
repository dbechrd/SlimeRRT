#pragma once
#include "args.h"
#include "error.h"
#include "world.h"
#include <thread>

struct GameServer {
    Args args;
    std::thread net_server_thread;

    World world{};

    GameServer(Args args);
    ErrorType Run();
};