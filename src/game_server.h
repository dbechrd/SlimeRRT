#pragma once
#include "world.h"
#include "args.h"
#include <thread>

struct GameServer {
    Args args;
    std::thread net_server_thread;

    World world{};

    GameServer(Args args);
};

void game_server_run(Args args);