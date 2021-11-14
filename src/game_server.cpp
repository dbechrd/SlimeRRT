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
    glfwInit();

    World *world = new World;
    //world->map = world->mapSystem.Generate(world->rtt_rand, 128, 128);
    world->map = world->mapSystem.Generate(world->rtt_rand, 256, 256);
    netServer.serverWorld = world;

    {
        // TODO: Move slime radius somewhere more logical.. some global table of magic numbers?
        const float slimeRadius = 50.0f;
        const size_t mapPixelsX = (size_t)world->map->width * TILE_W;
        const size_t mapPixelsY = (size_t)world->map->height * TILE_W;
        const float maxX = mapPixelsX - slimeRadius;
        const float maxY = mapPixelsY - slimeRadius;
        world->slimeCount = WORLD_ENTITIES_MAX;
        for (size_t i = 0; i < world->slimeCount; i++) {
            Slime &slime = world->slimes[i];
            slime.body.position.x = dlb_rand32f_range(slimeRadius, maxX);
            slime.body.position.y = dlb_rand32f_range(slimeRadius, maxY);
        }
    }

    E_ASSERT(netServer.OpenSocket(SERVER_PORT), "Failed to open socket");

    const double tps = 1.0f; //20.0f;
    const double dt = 1.0f / tps;
    double frameStart = glfwGetTime();
    double frameAccum = 0.0f;

    bool running = true;
    while (running) {
        E_ASSERT(netServer.Listen(), "Failed to listen on socket");

        double now = glfwGetTime();
        frameAccum += now - frameStart;
        frameStart = now;

        if (frameAccum > dt) {
            while (frameAccum > dt) {
                world->SimSlimes(now, dt);
                frameAccum -= dt;
            }
            E_ASSERT(netServer.BroadcastWorldPlayers(), "Failed to broadcast world players");
            E_ASSERT(netServer.BroadcastWorldEntities(), "Failed to broadcast world entities");
        }
    }

    delete world;
    return ErrorType::Success;
}
