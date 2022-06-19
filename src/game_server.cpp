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
    Catalog::g_items.LoadData();

    World *world = new World;
    world->map = world->mapSystem.Alloc(world->rtt_seed);

    for (short y = -2; y <= 2; y++) {
        for (short x = -2; x <= 2; x++) {
            world->map->FindOrGenChunk(*world, x, y);
        }
    }

    netServer.serverWorld = world;

    E_ASSERT(netServer.OpenSocket(args.port), "Failed to open socket");

    const double tickDt = SV_TICK_DT;
    double tickAccum = 0.0f;
    double frameStart = glfwGetTime();

    while (!args.serverQuit) {
        world->tick++;
        E_ASSERT(netServer.Listen(), "Failed to listen on socket");

        double now = glfwGetTime();
        tickAccum += now - frameStart;
        frameStart = now;

        while (tickAccum > tickDt) {
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
            assert(world->map);
            world->SV_Simulate(tickDt);

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

                // Keep track of how many milliseconds of input we've processed
                // for this client this tick.
                double inputSecAccum = 0;

                // Process queued player inputs
                for (size_t i = 0; i < client.inputHistory.Count(); i++) {
                    InputSample &sample = client.inputHistory.At(i);

                    if (sample.seq <= client.lastInputAck) {
                        //TraceLog(LOG_WARNING, "Ignoring old input #%u from %u\n", sample.seq, sample.ownerId);
                        continue;
                    }

                    double inputSec = sample.msec / 1000.0;
                    //double inputOverflow = (inputSecAccum + inputSec) - tickDt;
                    //if (inputOverflow > 0.0) {
                    //    sample.msec = (uint8_t)MIN(inputOverflow * 1000.0, UINT8_MAX);
                    //    TraceLog(LOG_WARNING, "Saving overflow (%u ms) from input.msec (seq: #%u, player: %u)", sample.msec, sample.seq, sample.ownerId);
                    //    //client.inputHistory.Clear();
                    //    break;
                    //}
                    inputSecAccum += inputSec;

                    player->Update(sample, *world->map);
                    client.lastInputAck = sample.seq;
                }

                double inputOverflow = inputSecAccum - tickDt;
                client.inputOverflow += inputOverflow;
                client.inputOverflow = MAX(0.0, client.inputOverflow);
#if SV_DEBUG_INPUT_SAMPLES
                TraceLog(LOG_DEBUG, "inputOverflow: %f, tickDt: %f%s", client.inputOverflow, tickDt,
                    client.inputOverflow > SV_INPUT_HACK_THRESHOLD ? " [INPUT HAX DETECTED]" : "");
#else
                if (client.inputOverflow > SV_INPUT_HACK_THRESHOLD) {
                    TraceLog(LOG_DEBUG, "inputOverflow: %f, tickDt: %f [INPUT HAX DETECTED]",
                        client.inputOverflow, tickDt);
                }
#endif

                // Send nearby chunks to player if they haven't received them yet
                netServer.SendNearbyChunks(client);

                if (glfwGetTime() - client.lastSnapshotSentAt > 1.0 / SNAPSHOT_SEND_RATE) {
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
            tickAccum -= tickDt;
        }
    }

    delete world;
    if (args.standalone) {
        glfwTerminate();
    }

    return ErrorType::Success;
}
