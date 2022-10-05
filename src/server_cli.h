#pragma once
#include "args.h"
#include "error.h"
#include "net_client.h"
#include "world.h"
#include <thread>

struct ServerCLI{
    ErrorType ParseArgs(int argc, char *argv[]) { return args.Parse(argc, argv); }
    ErrorType Run(const char *hostname, unsigned short port);

private:
    static const char *LOG_SRC;
    Args       args      {};
    NetClient  netClient {};
};