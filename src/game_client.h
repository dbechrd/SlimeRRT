#pragma once
#include "args.h"
#include "error.h"
#include "net_client.h"

struct World;

struct GameClient{
    GameClient(Args args) : args(args) {};
    ErrorType Run(const char *serverHost, unsigned short serverPort);

private:
    static const char *LOG_SRC;
    Args       args;
    World     *world     {};
    NetClient  netClient {};
};