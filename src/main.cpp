#define _CRTDBG_MAP_ALLOC

#include "args.h"
#include "error.h"
#include "game_client.h"
#include "game_server.h"
#include "server_cli.h"
#include "jail_win32_console.h"
#include "../test/tests.h"

DLB_ASSERT_HANDLER(handle_assert)
{
    TraceLog(LOG_FATAL,
        "\n---[DLB_ASSERT_HANDLER]-----------------\n"
        "Source file: %s:%d\n\n"
        "%s\n"
        "----------------------------------------\n",
        filename, line, expr
    );
    fflush(stdout);
    fflush(stderr);
    __debugbreak();
    assert(0);
    exit(-1);
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;

int main(int argc, char *argv[])
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF); //| _CRTDBG_CHECK_CRT_DF);
    //_CrtSetBreakAlloc(3569);

    glfwInit();

#if _DEBUG
    run_tests();
#endif

    Args args{ argc, argv };
    //args.standalone = true;

    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    GameServer *gameServer = 0;
#if 1
    if (args.standalone) {
        gameServer = new GameServer(args);
    }
#else
    gameServer = new GameServer(args);
#endif

    // TODO: Make CLI not be an entire client/player. Makes no sense for the CLI to show up in the world LUL.
    //const char *title = "[SLIMY SERVER]";
    //ServerCLI serverCli{ args };
    //serverCli.Run("localhost", SV_DEFAULT_PORT);

    if (!args.standalone) {
        int enet_code = enet_initialize();
        if (enet_code < 0) {
            TraceLog(LOG_FATAL, "Failed to initialize network utilities (enet). Error code: %d\n", enet_code);
        }

#ifdef _DEBUG
        InitConsole();
        // Dock left side of right monitor
        // { l:1913 t : 1 r : 2887 b : 1048 }
        SetConsolePosition(1913, 1, 2887 - 1913, 1048 - 1);
#endif

        error_init("game.log");

        InitWindow(1600, 900, "Attack the slimes!");
        // NOTE: I could avoid the flicker if Raylib would let me pass it as a flag into InitWindow -_-
        //SetWindowState(FLAG_WINDOW_HIDDEN);

        GameClient *game = new GameClient(args);
        game->Run();
        args.serverQuit = true;
        delete game;
    }

    //--------------------------------
    // Clean up
    //--------------------------------
    delete gameServer;
    error_free();
    CloseWindow();
    enet_deinitialize();
    return 0;
}

#pragma warning(disable:26819)
#define DLB_MURMUR3_IMPLEMENTATION
#include "dlb_murmur3.h"
#undef DLB_MURMUR3_IMPLEMENTATION
#pragma warning(default:26819)

#define DLB_RAND_IMPLEMENTATION
#define DLB_RAND_TEST
#include "dlb_rand.h"
#undef DLB_RAND_TEST
#undef DLB_RAND_IMPLEMENTATION

#pragma warning(push, 0)
//#pragma warning(disable:4244)  // converstion from float to int possible loss of data
//#pragma warning(disable:4303)  // Reading invalid data from guiIconsName
//#pragma warning(disable:4309)  // Possible buffer overrun
//#pragma warning(disable:6031)  // Return value ignored
//#pragma warning(disable:6835)  // Reading invalid data from guiIconsName
//#pragma warning(disable:26812)
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
#include "catalog/csv.cpp"
#include "catalog/items.cpp"
#include "catalog/particle_fx.cpp"
#include "catalog/sounds.cpp"
#include "catalog/spritesheets.cpp"
#include "catalog/tracks.cpp"
#include "chat.cpp"
#include "controller.cpp"
#include "draw_command.cpp"
#include "error.cpp"
#include "fx/blood.cpp"
#include "fx/gem.cpp"
#include "fx/gold.cpp"
#include "fx/golden_chest.cpp"
#include "fx/goo.cpp"
#include "fx/rainbow.cpp"
#include "game_client.cpp"
#include "game_server.cpp"
#include "healthbar.cpp"
#include "helpers.cpp"
#include "item_system.cpp"
#include "item_world.cpp"
#include "loot_table.cpp"
#include "net_client.cpp"
#include "net_message.cpp"
#include "net_server.cpp"
#include "net_stat.cpp"
#include "OpenSimplex2F.c"
#include "particles.cpp"
#include "perlin.cpp"
#include "player.cpp"
#include "server_cli.cpp"
#include "shadow.cpp"
#include "slime.cpp"
#include "sprite.cpp"
#include "spritesheet.cpp"
#include "spycam.cpp"
#include "structures/structure.cpp"
#include "tilemap.cpp"
#include "tileset.cpp"
#include "ui/ui.cpp"
#include "world.cpp"
#include "world_event.cpp"
#include "../test/tests.cpp"