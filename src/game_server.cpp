#include "game_server.h"
#include "net_server.h"

void game_server_run(Args args)
{
    GameServer gameServer{ args };

    // After everything loaded, start listening to network
    gameServer.net_server_thread = std::thread(net_server_run);

    // TODO: Maintain a queue of user input (broadcast chat immediately
    // or only on tick?), and ensure tick is delayed long enough to have
    // all of the input before the sim runs
}

GameServer::GameServer(Args args) : args(args)
{
};