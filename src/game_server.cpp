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
E_START
    World &world = netServer.serverWorld;
    //tilemap_generate_ex(&tilemap, world->rtt_rand, 128, 128);
    tilemap_generate_ex(world.map, world.rtt_rand, 254, 254);

    Tileset tileset{};
    tileset.tileWidth = 32;
    tileset.tileHeight = 32;
    world.tileset = &tileset;

    {
        const float slimeRadius = 50.0f;
        const size_t mapPixelsX = world.map.width * tileset.tileWidth;
        const size_t mapPixelsY = world.map.height * tileset.tileHeight;
        const float maxX = mapPixelsX - slimeRadius;
        const float maxY = mapPixelsY - slimeRadius;
        for (size_t i = 0; i < 256; i++) {
            world.slimes.emplace_back(nullptr, nullptr);
            Slime &slime = world.slimes.back();
            slime.body.position.x = dlb_rand32f_range(slimeRadius, maxX);
            slime.body.position.y = dlb_rand32f_range(slimeRadius, maxY);
        }
    }

    E_CHECK(netServer.OpenSocket(SERVER_PORT), "Failed to open socket");
    E_CHECK(netServer.Listen(), "Failed to listen on socket");
E_CLEAN_END
}
