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
        world->slimeCount = WORLD_ENTITIES_MAX;
        for (size_t i = 0; i < world->slimeCount; i++) {
            Slime &slime = world->slimes[i];
            world->InitSlime(slime);
        }
    }

    E_ASSERT(netServer.OpenSocket(SERVER_PORT), "Failed to open socket");

    const double dt = 1.0f / SERVER_TPS;
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
                // TODO: This is big mess.. need to simulate all players. Which order should they be processed in..?
                for (auto &kv : netServer.clients) {
                    NetServerClient &client = kv.second;
                    world->SimPlayer(now, dt, world->players[client.playerIdx], client.input);
                }
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
