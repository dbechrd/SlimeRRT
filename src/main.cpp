#include "args.h"
#include "bit_stream.h"
#include "chat.h"
#include "controller.h"
#include "draw_command.h"
#include "error.h"
#include "game_client.h"
#include "game_server.h"
#include "healthbar.h"
#include "helpers.h"
#include "item_catalog.h"
#include "maths.h"
#include "net_client.h"
#include "net_server.h"
#include "particles.h"
#include "particle_fx.h"
#include "player.h"
#include "slime.h"
#include "sound_catalog.h"
#include "spritesheet.h"
#include "spritesheet_catalog.h"
#include "tileset.h"
#include "tilemap.h"
#include "rtree.h"
#include "world.h"
#include "../test/tests.h"

#include "dlb_rand.h"
#include "raylib.h"
#include "raygui.h"
#include "gui_textbox_extended.h"
#include "zed_net.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <array>
#include <future>
#include <thread>

using namespace std::chrono_literals;

static const char *LOG_SRC = "Main";

int main(int argc, char *argv[])
{
#if _DEBUG
    run_tests();
#endif

    Args args{ argc, argv };
    //args.server = true;

    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    error_init();

    const char *title = "Attack the slimes!";
    if (args.server) {
        title = "[Open to LAN] Attack the slimes!";
    }
    InitWindow(1600, 900, title);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);  // Disable default Escape exit key, we'll handle escape ourselves

    if (zed_net_init() < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_FATAL, "Failed to initialize network utilities. Error: %s\n", err);
    }

    GameServer gameServer{ args };
    std::future gameServerFuture = std::async(std::launch::async, [&gameServer] {
        return gameServer.Run();
    });

    GameClient gameClient{ args };
#if 0
    gameClient.Run("slime.theprogrammingjunkie.com", 4040);
#else
    gameClient.Run("127.0.0.1", 4040);
#endif

    //--------------------------------
    // Clean up
    //--------------------------------
    zed_net_shutdown();
    return 0;
}

#pragma warning(push)
#pragma warning(disable:26819)
#define DLB_MURMUR3_IMPLEMENTATION
#include "dlb_murmur3.h"
#pragma warning(pop)