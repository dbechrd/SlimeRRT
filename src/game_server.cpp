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

    double tickAccum = 0.0f;
    g_clock.now = glfwGetTime();
    g_clock.server = true;

    while (!args.serverQuit) {
        E_ASSERT(netServer.Listen(), "Failed to listen on socket");

        // Limit delta time to prevent spiraling for after long hitches (e.g. hitting a breakpoint)
        double frameDt = MIN(glfwGetTime() - g_clock.now, SV_TICK_DT_MAX);

        // Time is of the essence
        g_clock.now += frameDt;
        tickAccum += frameDt;

        while (tickAccum > SV_TICK_DT) {
            world->tick++;
#if 0
            // TODO: Limit how many inputs player is allowed to send us each tick
            // Process queued player inputs
            size_t inputHistoryLen = netServer.inputHistory.Count();
            for (size_t i = 0; i < inputHistoryLen; i++) {
                InputSample &sample = netServer.inputHistory.At(i);

                SV_Client *client = netServer.FindClient(sample.ownerId);
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

                assert(client->playerId == player->type);
                assert(world->map);
                player->Update(tickDt, sample, *world->map);

                client->lastInputAck = sample.seq;
            }
#endif

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

                InputSample combinedSample{};
                combinedSample.ownerId = player->id;
                combinedSample.skipFx = true;

                // Process queued player inputs
                for (size_t i = 0; i < client.inputHistory.Count(); i++) {
                    InputSample &sample = client.inputHistory.At(i);

                    if (sample.seq <= client.lastInputAck) {
                        //TraceLog(LOG_WARNING, "Ignoring old input #%u from %u\n", sample.seq, sample.ownerId);
                        continue;
                    }
                    client.lastInputAck = sample.seq;

                    // TODO: Handle overflow somehow, though it doesn't really help the player to overflow this..
                    combinedSample.msec += sample.msec;

                    // If user wanted to walk/run/attack in any sample, keep that action
                    combinedSample.walkNorth |= sample.walkNorth;
                    combinedSample.walkEast  |= sample.walkEast;
                    combinedSample.walkSouth |= sample.walkSouth;
                    combinedSample.walkWest  |= sample.walkWest;
                    combinedSample.run       |= sample.run;
                    combinedSample.attack    |= sample.attack;
                    if (sample.selectSlot) {
                        combinedSample.selectSlot = sample.selectSlot;
                    }
                }

                player->Update(combinedSample, *world->map);

                assert(world->map);
                world->SV_Simulate(SV_TICK_DT);

                // Determine how many seconds of input we've processed
                // for this client this tick.
                double combinedInputSec = combinedSample.msec / 1000.0;
                double inputOverflow = combinedInputSec - SV_TICK_DT;
                client.inputOverflow += inputOverflow;
                client.inputOverflow = MAX(0.0, client.inputOverflow);
#if SV_DEBUG_INPUT_SAMPLES
                TraceLog(LOG_DEBUG, "inputOverflow: %f, tickDt: %f%s", client.inputOverflow, tickDt,
                    client.inputOverflow > SV_INPUT_HACK_THRESHOLD ? " [INPUT HAX DETECTED]" : "");
#else
                if (client.inputOverflow > SV_INPUT_HACK_THRESHOLD) {
                    TraceLog(LOG_DEBUG, "inputOverflow: %f, tickDt: %f [INPUT HAX DETECTED]",
                        client.inputOverflow, SV_TICK_DT);
                }
#endif

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

            world->DespawnDeadEntities();
            tickAccum -= SV_TICK_DT;
        }
    }

    delete world;
    if (args.standalone) {
        glfwTerminate();
    }

    return ErrorType::Success;
}
