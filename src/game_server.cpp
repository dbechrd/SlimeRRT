#include "game_server.h"
#include "net_server.h"

const char *GameServer::LOG_SRC = "GameServer";

GameServer::GameServer(const Args *args)
{
    serverThread = new std::thread([this, args] {
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

ErrorType GameServer::Run(const Args *args)
{
    g_clock.server = true;
    error_init("server.log");

#if 0
    if (args.standalone) {
        // Load data that GameClient would have already loaded otherwise
        g_item_catalog.LoadData();
    }
#else
    // This is per-thread now.. I don't think sharing it is a good idea?
    g_item_catalog.LoadData();
#endif

    World *world = new World;

    // Pre-generate spawn chunks
    for (short y = -2; y <= 2; y++) {
        for (short x = -2; x <= 2; x++) {
            world->map.FindOrGenChunk(*world, x, y);
        }
    }

    //world->SpawnNpc(0, NPC::Type_Townfolk, { 0, 0 }, 0);

    netServer.serverWorld = world;

    E_ERROR_RETURN(netServer.OpenSocket(args->port), "Failed to open socket", 0);

    while (!args->serverQuit) {
        E_ERROR_RETURN(netServer.Listen(), "Failed to listen on socket", 0);

        const double now = glfwGetTime();
        const double dt = now - g_clock.nowPrev;
        g_clock.nowPrev = now;
        g_clock.accum += MIN(SV_TICK_DT_ACCUM_MAX, dt);  // Limit accumulator

        //printf("clock: %f now: %f tickDt: %f dtAccum: %f\n", g_clock.now, now, tickDt, dtAccum);
        assert(dt > 0);

        // Check if tick due
        if (g_clock.accum > SV_TICK_DT) {
            g_clock.accum -= SV_TICK_DT;

            // Time is of the essence
            g_clock.now += SV_TICK_DT;
            world->tick++;
            //E_DEBUG("tick: %u now: %f clock: %f dt: %f", world->tick, now, g_clock.now, dt);
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
                    E_DEBUG("Player not found, cannot simulate", 0);
                    continue;
                }
                assert(client.playerId == player->id);

#if 1
                // Ignore inputs that are too far behind to ever get processed to avoid excessive input processing latency
                {
                    float dtAccum = 0;
                    uint32_t oldestInputSeqToProcess = client.lastInputAck;
                    int inputHistoryLen = (int)client.inputHistory.Count();
                    for (int i = inputHistoryLen - 1; i >= 0 && dtAccum < (float)SV_INPUT_HISTORY_DT_MAX; i--) {
                        InputSample input = client.inputHistory.At(i);
                        if (input.seq <= client.lastInputAck) {
                            break;
                        }

                        oldestInputSeqToProcess = input.seq;
                        dtAccum += input.dt;
                    }
                    if (oldestInputSeqToProcess > client.lastInputAck + 1) {
                        int first = client.lastInputAck + 1;
                        int last = oldestInputSeqToProcess - 1;
                        int count = (last - first) + 1;
                        E_WARN("SVR [tick: %u] discard old input: %u - %u (%u samples)", world->tick, first, last, count);
                        client.lastInputAck = oldestInputSeqToProcess - 1;
                    }
                }

                // Process up to 1 tick's timespan worth of queued input
                {
                    size_t inputHistoryLen = client.inputHistory.Count();
                    for (size_t i = 0; i < inputHistoryLen; i++) {
                        InputSample input = client.inputHistory.At(i);
                        if (input.seq <= client.lastInputAck) {
                            //E_WARN("Ignoring old input #%u from %u\n", input.seq, input.ownerId);
                            continue;
                        }

                        // This is good enough for now to prevent speed hax. If a vanilla client can't
                        // manage to send us at least one input sample per server tick, fuck 'em.
                        input.dt = MIN(input.dt, (float)CL_INPUT_SEND_RATE_LIMIT_DT);
                        client.lastInputAck = input.seq;
                        player->Update(input, world->map);
                    }

                    //if (client.inputOverflow) {
                    //    E_DEBUG("tick %u input overflow %.3f", world->tick, client.inputOverflow);
                    //}
                }
#else
                size_t inputHistoryLen = client.inputHistory.Count();
                for (size_t i = 0; i < inputHistoryLen; i++) {
                    InputSample input = client.inputHistory.At(i);
                    if (input.seq <= client.lastInputAck) {
                        //E_WARN("Ignoring old input #%u from %u\n", input.seq, input.ownerId);
                        continue;
                    }
                    player->Update(input, world->map);
                    client.lastInputAck = input.seq;
                    break;
                }
#endif
            }

            // Run server tasks
            world->SV_DespawnDeadEntities();
            world->SV_Simulate(SV_TICK_DT);

            // Send players world updates
            for (size_t i = 0; i < SV_MAX_PLAYERS; i++) {
                SV_Client &client = netServer.clients[i];
                if (!client.playerId) {
                    continue;
                }

                // Send nearby chunks to player if they haven't received them yet
                netServer.SendNearbyChunks(client);

                if (g_clock.now - client.lastSnapshotSentAt > SNAPSHOT_SEND_DT) {
    #if SV_DEBUG_INPUT_SAMPLES
                    E_DEBUG("Sending snapshot for tick %u / input seq #%u, to player %u\n", world->tick, client.lastInputAck, client.playerId);
    #endif
                    // Send snapshot
                    E_ERROR_RETURN(netServer.SendWorldSnapshot(client), "Failed to send world snapshot", 0);
                } else {
                    //E_DEBUG("Skipping shapshot for %u", client.playerId);
                }

                // TODO: Send global events

                // Send nearby events
                //E_ASSERT(netServer.SendNearbyEvents(client), "Failed to send nearby events. playerId: %u", client.playerId);
            }
        }
    }

    delete world;
    if (args->standalone) {
        glfwTerminate();
    }

    error_free();
    return ErrorType::Success;
}
