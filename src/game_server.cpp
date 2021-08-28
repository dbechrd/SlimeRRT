#include "game_server.h"
#include "net_server.h"
#include <chrono>
#include <future>

using namespace std::chrono_literals;

static const char *LOG_SRC = "GameServer";

GameServer::GameServer(Args args) : args(args)
{
};

ErrorType GameServer::Run()
{
E_START
    std::future future = std::async(std::launch::async, []{ return net_server_run(); });

    while (1) {
        if (future.wait_for(0ms) == std::future_status::ready) {
            // TODO: Check for expected thread end (e.g. when server closed by request)
            E_CHECK(future.get(), "NetServer thread died");
            break;
        }

        // TODO: Maintain a queue of user input (broadcast chat immediately
        // or only on tick?), and ensure tick is delayed long enough to have
        // all of the input before the sim runs
    }
E_CLEAN_END
}
