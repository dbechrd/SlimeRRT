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
    Args args;
    World world         {};
    NetServer netServer {};
};