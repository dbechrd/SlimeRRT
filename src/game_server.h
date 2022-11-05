#pragma once
#include "args.h"
#include "error.h"
#include "net_server.h"
#include <thread>

struct GameServer {
    GameServer(const Args *args);
    ~GameServer();
    ErrorType Run(const Args *args);

private:
    static const char *LOG_SRC;
    std::thread *serverThread {};
    NetServer    netServer    {};
};