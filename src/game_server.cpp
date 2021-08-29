#include "game_server.h"
#include "net_server.h"
#include <chrono>
#include <future>

using namespace std::chrono_literals;

static const char *LOG_SRC = "GameServer";
const unsigned int SERVER_PORT = 4040;

GameServer::GameServer(Args args) : args(args)
{
};

ErrorType GameServer::Run()
{
E_START
    {
        E_CHECK(netServer.OpenSocket(SERVER_PORT), "Failed to open socket");
        std::future future = std::async(std::launch::async, [this]{ return netServer.Listen(); });

        while (1) {
            if (future.wait_for(0ms) == std::future_status::ready) {
                // TODO: Check for expected thread end (e.g. when server closed by request)
                break;
            }

            // TODO: Maintain a queue of user input (broadcast chat immediately
            // or only on tick?), and ensure tick is delayed long enough to have
            // all of the input before the sim runs

            std::this_thread::sleep_for(10ms);
        }
        E_CHECK(future.get(), "Listen thread died");
    }
E_CLEAN_END
}
