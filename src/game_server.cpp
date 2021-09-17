#include "game_server.h"
#include "net_server.h"
#include <chrono>
#include <future>

using namespace std::chrono_literals;

const char *GameServer::LOG_SRC = "GameServer";

GameServer::GameServer(Args args) : args(args)
{
};

ErrorType GameServer::Run()
{
    E_START
    {
        World serverWorld{};
        world = &serverWorld;
        //tilemap_generate_ex(&tilemap, 128, 128, &tileset);
        tilemap_generate_ex(&world->map, 256, 256, 32, 32, &world->rtt_rand);
        //tilemap_generate_ex(&tilemap, 512, 512, &tileset);

        {
            const float slimeRadius = 50.0f;
            const size_t mapPixelsX = world->map.width * world->map.tileWidth;
            const size_t mapPixelsY = world->map.height * world->map.tileHeight;
            const float maxX = mapPixelsX - slimeRadius;
            const float maxY = mapPixelsY - slimeRadius;
            for (size_t i = 0; i < 256; i++) {
                world->slimes.emplace_back(nullptr, nullptr);
                Slime &slime = world->slimes.back();
                slime.body.position.x = dlb_rand32f_range(slimeRadius, maxX);
                slime.body.position.y = dlb_rand32f_range(slimeRadius, maxY);
            }
        }

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
