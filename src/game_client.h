#pragma once
#include "args.h"
#include "error.h"
#include "net_client.h"

struct World;

struct GameClient{
    GameClient(const Args &args) : args(args) {};
    ErrorType Run(void);

private:
    static const char *LOG_SRC;
    const Args &args;
    World      *world    {};
    NetClient  netClient {};
};