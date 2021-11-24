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
        for (size_t i = 0; i < SV_MAX_ENTITIES; i++) {
            Slime &slime = world->slimes[i];
            uint32_t slimeId = 0;
            world->SpawnSlime(0);
        }
    }

    E_ASSERT(netServer.OpenSocket(SV_DEFAULT_PORT), "Failed to open socket");

    const double tickDt = 1.0f / SV_TICK_RATE;
    double tickAccum = 0.0f;
    double frameStart = glfwGetTime();

    bool running = true;
    while (running) {
        E_ASSERT(netServer.Listen(), "Failed to listen on socket");

        double now = glfwGetTime();
        tickAccum += now - frameStart;
        frameStart = now;

        while (tickAccum > tickDt) {
            // Process queued player inputs
            size_t inputHistoryLen = netServer.inputHistory.Count();
            for (size_t i = 0; i < inputHistoryLen; i++) {
                InputSample &sample = netServer.inputHistory.At(i);

                NetServerClient *client = netServer.FindClient(sample.ownerId);
                if (!client) {
                    continue;
                }
                //assert(client->lastInputAck < world->tick);

                if (sample.seq <= client->lastInputAck) {
                    // TODO: If < world->tick, discard from queue
                    continue;
                }

                Player *player = world->FindPlayer(sample.ownerId);
                if (!player) {
                    continue;
                }

                assert(client->playerId == player->id);
                assert(world->map);
                player->Update(tickDt, sample, *world->map);

                client->lastInputAck = sample.seq;
            }

            world->Simulate(tickDt);

            for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
                NetServerClient &client = netServer.clients[i];
                if (client.playerId && (glfwGetTime() - client.lastSnapshotSentAt) > (1000.0 / SNAPSHOT_SEND_RATE) / 1000.0) {
                    printf("Sending snapshot for tick %u to player %u\n", world->tick, client.playerId);
                    WorldSnapshot &worldSnapshot = client.worldHistory.Alloc();
                    assert(!worldSnapshot.tick);  // ringbuffer alloc fucked up and didn't zero slot
                    worldSnapshot.playerId = client.playerId;
                    worldSnapshot.lastInputAck = client.lastInputAck;
                    worldSnapshot.tick = world->tick;
                    world->GenerateSnapshot(worldSnapshot);
                    E_ASSERT(netServer.SendWorldSnapshot(client, worldSnapshot), "Failed to broadcast world snapshot");
                }
            }

            world->tick++;
            tickAccum -= tickDt;
        }
    }

    delete world;
    return ErrorType::Success;
}
