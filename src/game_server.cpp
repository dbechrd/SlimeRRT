#include "game_server.h"
#include "net_server.h"
#include <chrono>
#include <future>

using namespace std::chrono_literals;

const char *GameServer::LOG_SRC = "GameServer";

ErrorType GameServer::Run()
{
    if (args.standalone) {
        glfwInit();
    }

    Catalog::g_items.Load();

    World *world = new World;
    //world->map = world->mapSystem.Generate(world->rtt_rand, 128, 128);
    world->map = world->mapSystem.Generate(world->rtt_rand, 256, 256);
    netServer.serverWorld = world;

    {
        for (size_t i = 0; i < SV_MAX_SLIMES; i++) {
            Slime &slime = world->slimes[i];
            uint32_t slimeId = 0;
            world->SpawnSlime(0);
            //slime.body.position = world->GetWorldSpawn();
            //slime.body.position.y -= 50;
        }
    }

    E_ASSERT(netServer.OpenSocket(args.port), "Failed to open socket");

    const double tickDt = 1.0f / SV_TICK_RATE;
    double tickAccum = 0.0f;
    double frameStart = glfwGetTime();

    while (!args.exiting) {
        world->tick++;
        E_ASSERT(netServer.Listen(), "Failed to listen on socket");

        double now = glfwGetTime();
        tickAccum += now - frameStart;
        frameStart = now;

        while (tickAccum > tickDt) {
            // TODO: Limit how many inputs player is allowed to send us each tick
            // Process queued player inputs
            size_t inputHistoryLen = netServer.inputHistory.Count();
            for (size_t i = 0; i < inputHistoryLen; i++) {
                InputSample &sample = netServer.inputHistory.At(i);

                NetServerClient *client = netServer.FindClient(sample.ownerId);
                if (!client) {
                    //printf("Client not found, cannot sample input #%u from %u\n", sample.seq, sample.ownerId);
                    continue;
                }

                if (sample.seq <= client->lastInputAck) {
                    //printf("Ignoring old input #%u from %u\n", sample.seq, sample.ownerId);
                    continue;
                }

                Player *player = world->FindPlayer(sample.ownerId);
                if (!player) {
                    printf("Player not found, cannot sample input #%u from %u\n", sample.seq, sample.ownerId);
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
#if SV_DEBUG_INPUT
                    printf("Sending snapshot for tick %u / input seq #%u, to player %u\n", world->tick, client.lastInputAck, client.playerId);
#endif
                    WorldSnapshot &worldSnapshot = client.worldHistory.Alloc();
                    assert(!worldSnapshot.tick);  // ringbuffer alloc fucked up and didn't zero slot
                    worldSnapshot.playerId = client.playerId;
                    worldSnapshot.lastInputAck = client.lastInputAck;
                    worldSnapshot.tick = world->tick;
                    world->GenerateSnapshot(worldSnapshot);
                    E_ASSERT(netServer.SendWorldSnapshot(client, worldSnapshot), "Failed to broadcast world snapshot");
                }
            }

            tickAccum -= tickDt;
        }
    }

    delete world;
    if (args.standalone) {
        glfwTerminate();
    }
    return ErrorType::Success;
}
