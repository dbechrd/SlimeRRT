#include "args.h"
#include "error.h"
#include "game_client.h"
#include "game_server.h"
#include "server_cli.h"
#include "jail_win32_console.h"
#include "../test/tests.h"

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

    int enet_code = enet_initialize();
    if (enet_code < 0) {
        TraceLog(LOG_FATAL, "Failed to initialize network utilities (enet). Error code: %d\n", enet_code);
    }

    std::thread serverThread([&args] {
        if (args.server) {
            GameServer *gameServer = new GameServer(args);
            gameServer->Run();
            delete gameServer;
        }
    });

    if (args.server) {
        // TODO: Make CLI not be an entire client/player. Makes no sense for the CLI to show up in the world LUL.
        //ServerCLI serverCli{ args };
        //serverCli.Run("localhost", SV_DEFAULT_PORT);
        //AllocConsole();
#ifdef _DEBUG
        // Dock right side of right monitor
        // { l:2873 t : 1 r : 3847 b : 1048 }
        SetConsolePosition(2873, 1, 3847 - 2873, 1048 - 1);
#endif
    } else {
#ifdef _DEBUG
        // { l:1913 t : 1 r : 2887 b : 1048 }
        SetConsolePosition(1913, 1, 2887 - 1913, 1048 - 1);
#else
        HideConsole();
#endif
        GameClient *gameClient = new GameClient(args);
        gameClient->Run();
        delete gameClient;
    }

    //--------------------------------
    // Clean up
    //--------------------------------
    serverThread.join();
    enet_deinitialize();
    return 0;
}

#pragma warning(push)
#pragma warning(disable:26819)
#define DLB_MURMUR3_IMPLEMENTATION
#include "dlb_murmur3.h"
#undef DLB_MURMUR3_IMPLEMENTATION
#pragma warning(pop)

#define DLB_RAND_IMPLEMENTATION
#define DLB_RAND_TEST
#include "dlb_rand.h"
#undef DLB_RAND_TEST
#undef DLB_RAND_IMPLEMENTATION

#pragma warning(push)
#pragma warning(disable:4244)  // converstion from float to int possible loss of data
#pragma warning(disable:6031)  // Return value ignored
#pragma warning(disable:26812)
#define RAYGUI_IMPLEMENTATION
#include "raylib/raygui.h"
#undef RAYGUI_IMPLEMENTATION
#pragma warning(pop)

#if 0
#pragma warning(push)
#pragma warning(disable: 4244)  // conversion from 'int' to 'float'
#pragma warning(disable: 4267)  // conversion from 'size_t' to 'int'
#define GUI_TEXTBOX_EXTENDED_IMPLEMENTATION
#include "gui_textbox_extended.h"
#undef GUI_TEXTBOX_EXTENDED_IMPLEMENTATION
#pragma warning(pop)
#endif

#include "args.cpp"
#include "bit_stream.cpp"
#include "body.cpp"
#include "catalog/items.cpp"
#include "catalog/mixer.cpp"
#include "catalog/particle_fx.cpp"
#include "catalog/sounds.cpp"
#include "catalog/spritesheets.cpp"
#include "catalog/tracks.cpp"
#include "chat.cpp"
#include "controller.cpp"
#include "draw_command.cpp"
#include "error.cpp"
#include "game_client.cpp"
#include "game_server.cpp"
#include "healthbar.cpp"
#include "helpers.cpp"
#include "loot_table.cpp"
#include "maths.cpp"
#include "net_client.cpp"
#include "net_server.cpp"
#include "net_message.cpp"
#include "fx/blood.cpp"
#include "fx/gem.cpp"
#include "fx/gold.cpp"
#include "fx/golden_chest.cpp"
#include "fx/goo.cpp"
#include "particles.cpp"
#include "player.cpp"
#include "server_cli.cpp"
#include "shadow.cpp"
#include "slime.cpp"
#include "sprite.cpp"
#include "spritesheet.cpp"
#include "structures/structure.cpp"
#include "tilemap.cpp"
#include "tileset.cpp"
#include "ui/ui.cpp"
#include "world.cpp"
#include "../test/tests.cpp"