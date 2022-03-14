#include "game_server.h"
#include "net_server.h"
#include <chrono>
#include <future>

using namespace std::chrono_literals;

const char *GameServer::LOG_SRC = "GameServer";

std::thread &GameServer::StartThread(const Args &args)
{
    std::thread *serverThread = new std::thread([&args] {
        GameServer *gameServer = new GameServer();
        gameServer->Run(args);
        delete gameServer;
    });
    return *serverThread;
}

ErrorType GameServer::Run(const Args &args)
{
#ifdef _DEBUG
    InitConsole();
    // Dock right side of right monitor
    // { l:2873 t : 1 r : 3847 b : 1048 }
    //SetConsolePosition(2873, 1, 3847 - 2873, 1048 - 1);
#endif

    int enet_code = enet_initialize();
    if (enet_code < 0) {
        TraceLog(LOG_FATAL, "Failed to initialize network utilities (enet). Error code: %d\n", enet_code);
    }

    error_init("server.log");

    Catalog::g_items.Load();

    World *world = new World;
    world->map = world->mapSystem.Alloc();

    //world->map->FindOrGenChunk(world->rtt_seed, -1, 0);

    for (short y = -2; y < 2; y++) {
        for (short x = -2; x < 2; x++) {
            world->map->FindOrGenChunk(world->rtt_seed, x, y);
        }
    }

    netServer.serverWorld = world;

    {
        Slime *slime = world->SpawnSlime(0);
        slime->body.Teleport(world->GetWorldSpawn());
        slime->body.Move({ -50.0f, 0 });

        slime = world->SpawnSlime(0);
        slime->body.Teleport(world->GetWorldSpawn());
        slime->body.Move({ -50.0f, 50.0f });

        for (size_t i = 0; i < SV_MAX_SLIMES; i++) {
            world->SpawnSlime(0);
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

            world->SV_Simulate(tickDt);

            for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
                NetServerClient &client = netServer.clients[i];
                if (!client.playerId) {
                    continue;
                }

                // Send nearby chunks to player if they haven't received them yet
                netServer.SendNearbyChunks(client);

                if (glfwGetTime() - client.lastSnapshotSentAt > 1.0 / SNAPSHOT_SEND_RATE) {
#if SV_DEBUG_INPUT
                    printf("Sending snapshot for tick %u / input seq #%u, to player %u\n", world->tick, client.lastInputAck, client.playerId);
#endif
                    // Send snapshot
                    E_ASSERT(netServer.SendWorldSnapshot(client), "Failed to send world snapshot");
                } else {
                    //TraceLog(LOG_DEBUG, "Skipping shapshot for %u", client.playerId);
                }

                // TODO: Send global events

                // Send nearby events
                //E_ASSERT(netServer.SendNearbyEvents(client), "Failed to send nearby events. playerId: %u", client.playerId);
            }

            world->DespawnDeadEntities();
            tickAccum -= tickDt;
        }
    }

    delete world;
    if (args.standalone) {
        glfwTerminate();
    }

    return ErrorType::Success;
}
