#pragma once
#include "args.h"
#include "error.h"
#include "net_client.h"
#include "world.h"
#include <thread>

struct GameClient{
    GameClient(Args args) : args(args) {};
    ErrorType Run(const char *hostname, unsigned short port);

private:
    static const char *LOG_SRC;
    Args args;
    World world         {};
    NetClient netClient {};
};