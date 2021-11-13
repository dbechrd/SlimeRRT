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
    World *world = new World;
    //world->map = world->mapSystem.Generate(world->rtt_rand, 128, 128);
    world->map = world->mapSystem.Generate(world->rtt_rand, 254, 254);
    netServer.serverWorld = world;

    {
        // TODO: Move slime radius somewhere more logical.. some global table of magic numbers?
        const float slimeRadius = 50.0f;
        const size_t mapPixelsX = (size_t)world->map->width * TILE_W;
        const size_t mapPixelsY = (size_t)world->map->height * TILE_W;
        const float maxX = mapPixelsX - slimeRadius;
        const float maxY = mapPixelsY - slimeRadius;
        for (size_t i = 0; i < 256; i++) {
            world->slimes.emplace_back(nullptr, nullptr);
            Slime &slime = world->slimes.back();
            slime.body.position.x = dlb_rand32f_range(slimeRadius, maxX);
            slime.body.position.y = dlb_rand32f_range(slimeRadius, maxY);
        }
    }

    E_ASSERT(netServer.OpenSocket(SERVER_PORT), "Failed to open socket");

    bool running = true;
    while (running) {
        E_ASSERT(netServer.Listen(), "Failed to listen on socket");
    }

    delete world;
    return ErrorType::Success;
}
