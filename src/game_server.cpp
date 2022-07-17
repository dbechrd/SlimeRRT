#include "game_server.h"
#include "net_server.h"
#include <chrono>
#include <future>

using namespace std::chrono_literals;

const char *GameServer::LOG_SRC = "GameServer";

GameServer::GameServer(const Args &args)
{
    serverThread = new std::thread([this, &args] {
        Run(args);
    });
}

GameServer::~GameServer()
{
    if (serverThread) {
        serverThread->join();
    }
    delete serverThread;
}

ErrorType GameServer::Run(const Args &args)
{
    if (args.standalone) {
        // Load data that GameClient would have already loaded otherwise
        Catalog::g_items.LoadData();
    }

    World *world = new World;
    world->map = world->mapSystem.Alloc();
    world->map->SeedSimplex(world->rtt_seed);

    for (short y = -2; y <= 2; y++) {
        for (short x = -2; x <= 2; x++) {
            world->map->FindOrGenChunk(*world, x, y);
        }
    }

    netServer.serverWorld = world;

    E_ASSERT(netServer.OpenSocket(args.port), "Failed to open socket");

    g_clock.server = true;
    g_clock.now = 0;

    while (!args.serverQuit) {
        E_ASSERT(netServer.Listen(), "Failed to listen on socket");

        // Check if tick due
        const double now = glfwGetTime();
        const double tickDt = now - g_clock.now;
        if (tickDt > SV_TICK_DT) {
            // Time is of the essence
            g_clock.now += SV_TICK_DT;
            world->tick++;
            //TraceLog(LOG_DEBUG, "Sim tick %u now: %f clock %f", world->tick, now, g_clock.now);
#if 0
            // DEBUG: Drop all client inputs if server was paused for too long in debugger
            if (tickDt > SV_DEBUG_TICK_DT_MAX) {
                for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
                    SV_Client &client = netServer.clients[i];
                    if (!client.playerId) {
                        continue;
                    }

                    if (client.inputHistory.Count()) {
                        client.lastInputAck = client.inputHistory.Last().seq;
                        client.inputHistory.Clear();
                    }
                }
            }
#endif
            // Process players' input
            for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
                SV_Client &client = netServer.clients[i];
                if (!client.playerId) {
                    continue;
                }

                Player *player = world->FindPlayer(client.playerId);
                if (!player) {
                    TraceLog(LOG_DEBUG, "Player not found, cannot simulate");
                    continue;
                }
                assert(client.playerId == player->id);

                // Ignore inputs that are too far behind to ever get processed to avoid excessive input processing latency
                {
                    float dtAccum = 0;
                    uint32_t oldestInputSeqToProcess = client.lastInputAck;
                    int inputHistoryLen = (int)client.inputHistory.Count();
                    for (int i = inputHistoryLen - 1; i >= 0 && dtAccum < SV_INPUT_HISTORY_DT_MAX; i--) {
                        InputSample input = client.inputHistory.At(i);
                        if (input.seq <= client.lastInputAck) {
                            continue;
                        }

                        oldestInputSeqToProcess = input.seq;
                        dtAccum += input.dt;
                    }
                    if (oldestInputSeqToProcess > client.lastInputAck + 1) {
                        int first = client.lastInputAck + 1;
                        int last = oldestInputSeqToProcess - 1;
                        int count = (last - first) + 1;
                        TraceLog(LOG_DEBUG, "SVR tick: %u discard old input seq: %u - %u (%u samples)", world->tick, first, last, count);
                        client.lastInputAck = last;
                    }
                }

                // Process up to 1 tick's timespan worth of queued input
                {
                    float processedDt = 0;
                    size_t inputHistoryLen = client.inputHistory.Count();
                    for (size_t i = 0; i < inputHistoryLen && processedDt < SV_TICK_DT; i++) {
                        InputSample input = client.inputHistory.At(i);
                        if (input.seq <= client.lastInputAck) {
                            //TraceLog(LOG_WARNING, "Ignoring old input #%u from %u\n", sample.seq, sample.ownerId);
                            continue;
                        }

    #if 1
                        if (client.inputOverflow) {
                            // Consume partial input from previous tick
                            input.dt = client.inputOverflow;
                            client.inputOverflow = 0;
                        }

                        processedDt += input.dt;

                        if (processedDt <= SV_TICK_DT) {
                            // Consume whole input
                            client.lastInputAck = input.seq;
                        } else {
                            // Consume partial input at end of tick if we ran out of time
                            client.inputOverflow = (float)(processedDt - SV_TICK_DT);
                            input.dt -= client.inputOverflow;
                        }
    #else
                        processedDt += input.dt;
                        client.lastInputAck = input.seq;
    #endif

                        //TraceLog(LOG_DEBUG, "SVR SQ: %u OS: %f S: %f", input.seq, origInput.dt, input.dt);
                        player->Update(input, *world->map);
                    }
                }

                assert(world->map);
                world->SV_Simulate(SV_TICK_DT);

                // Send nearby chunks to player if they haven't received them yet
                netServer.SendNearbyChunks(client);

                if (g_clock.now - client.lastSnapshotSentAt > SNAPSHOT_SEND_DT) {
    #if SV_DEBUG_INPUT_SAMPLES
                    TraceLog(LOG_DEBUG, "Sending snapshot for tick %u / input seq #%u, to player %u\n", world->tick, client.lastInputAck, client.playerId);
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

            // Run server tasks
            world->DespawnDeadEntities();
        }
    }

    delete world;
    if (args.standalone) {
        glfwTerminate();
    }

    return ErrorType::Success;
}
