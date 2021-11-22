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
        for (size_t i = 0; i < WORLD_ENTITIES_MAX; i++) {
            Slime &slime = world->slimes[i];
            uint32_t slimeId = 0;
            world->SpawnSlime(0);
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
                // Process queued player inputs
                for (size_t i = 0; i < netServer.inputHistory.Count(); i++) {
                    InputSnapshot &inputSnapshot = netServer.inputHistory.At(i);
                    Player *player = world->FindPlayer(inputSnapshot.ownerId);
                    if (player) {
                        assert(world->map);
                        player->ProcessInput(inputSnapshot, *world->map);
                        NetServerClient *client = netServer.FindClient(player->id);
                        assert(client);
                        if (client) {
                            client->inputSeq = inputSnapshot.seq;
                        }
                    }
                }

                world->Simulate(now, dt);

                for (size_t i = 0; i < SERVER_MAX_PLAYERS; i++) {
                    NetServerClient &client = netServer.clients[i];
                    if (client.playerId) {
                        WorldSnapshot &worldSnapshot = client.worldHistory.Alloc();
                        assert(!worldSnapshot.tick);  // ringbuffer alloc fucked up and didn't zero slot
                        worldSnapshot.playerId = client.playerId;
                        worldSnapshot.inputSeq = client.inputSeq;
                        worldSnapshot.tick = world->tick;
                        world->GenerateSnapshot(worldSnapshot);
                        E_ASSERT(netServer.SendWorldSnapshot(client, worldSnapshot), "Failed to broadcast world snapshot");
                    }
                }

                world->tick++;
                frameAccum -= dt;
            }
        }
    }

    delete world;
    return ErrorType::Success;
}
