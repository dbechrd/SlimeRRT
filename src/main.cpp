#define _CRTDBG_MAP_ALLOC

#include "args.h"
#include "error.h"
#include "game_client.h"
#include "game_server.h"
#include "server_cli.h"
#include "jail_win32_console.h"
//#include "dlb_memory.h"
#include "../test/tests.h"
#include "GLFW/glfw3.h"

DLB_ASSERT_HANDLER(handle_assert)
{
    TraceLog(LOG_FATAL,
        "\n---[DLB_ASSERT_HANDLER]-----------------\n"
        "Source file: %s:%d\n\n"
        "%s\n"
        "----------------------------------------\n",
        filename, line, expr
    );
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;

int YourAllocHook(int nAllocType, void *pvData, size_t nSize, int nBlockUse, long lRequest,
    const unsigned char *szFileName, int nLine)
{
    if (nBlockUse == _CRT_BLOCK || nBlockUse == _IGNORE_BLOCK) return true;
    if (nAllocType == _HOOK_FREE) return true;
    //if (lRequest > 200) return true;

    const char *allocType = 0;
    switch (nAllocType) {
        case _HOOK_ALLOC:   allocType = "  alloc"; break;
        case _HOOK_REALLOC: allocType = "realloc"; break;
        case _HOOK_FREE:    allocType = "   free"; break;
    }

    const char *blockUse = 0;
    switch (nBlockUse) {
        case _FREE_BLOCK  : blockUse = "  FREE"; break;
        case _NORMAL_BLOCK: blockUse = "NORMAL"; break;
        case _CRT_BLOCK   : blockUse = "   CRT"; break;
        case _IGNORE_BLOCK: blockUse = "IGNORE"; break;
        case _CLIENT_BLOCK: blockUse = "CLIENT"; break;
    }

    _CrtSetAllocHook(0);
    printf("[%s:%4d]{%6d}[%s][%s] %p (%zu bytes)\n", szFileName, nLine, lRequest, allocType, blockUse, pvData, nSize);
    _CrtSetAllocHook(YourAllocHook);

    return true;
}

#define SET_CRT_DEBUG_FIELD(a) \
    _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define CLEAR_CRT_DEBUG_FIELD(a) \
    _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

int main(int argc, char *argv[])
{
    //for (int i = 0; i < 1000; i++) {
    //    _CrtSetBreakAlloc(i);
    //}
    //_CrtSetAllocHook(YourAllocHook);
    //_CrtSetBreakAlloc(13839);

#if 0
    // Send all reports to STDOUT
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

    SET_CRT_DEBUG_FIELD(_CRTDBG_ALLOC_MEM_DF);
    SET_CRT_DEBUG_FIELD(_CRTDBG_CHECK_ALWAYS_DF);
    //SET_CRT_DEBUG_FIELD(_CRTDBG_LEAK_CHECK_DF);
    CLEAR_CRT_DEBUG_FIELD(_CRTDBG_LEAK_CHECK_DF);
    CLEAR_CRT_DEBUG_FIELD(_CRTDBG_CHECK_CRT_DF);
#endif

    glfwInit();

#if RUN_TESTS
    run_tests();
#endif

    Args args{ argc, argv };
    //args.standalone = true;

    int enet_code = enet_initialize();
    if (enet_code < 0) {
        TraceLog(LOG_FATAL, "Failed to initialize network utilities (enet). Error code: %d\n", enet_code);
    }

    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    GameServer *gameServer = 0;
    if (args.standalone) {
#ifdef _DEBUG
        InitConsole(2873, 1, 3847 - 2873, 1048 - 1);  // Dock right side of right monitor
#endif
        gameServer = new GameServer(args);

        // TODO: Make CLI not be an entire client/player. Makes no sense for the CLI to show up in the world LUL.
        //const char *title = "[SLIMY SERVER]";
        //ServerCLI serverCli{ args };
        //serverCli.Run("localhost", SV_DEFAULT_PORT);
    } else {
#ifdef _DEBUG
        InitConsole(1913, 1, 2887 - 1913, 1048 - 1);  // Dock left side of right monitor
#endif

        GameClient *game = new GameClient(args);
        game->Run();
        args.serverQuit = true;
        delete game;
    }

    //--------------------------------
    // Clean up
    //--------------------------------
    delete gameServer;
    CloseWindow();
    enet_deinitialize();

#if 0
    if (_CrtDumpMemoryLeaks()) {
        fflush(stdout);
        fflush(stderr);
        //UNUSED(getchar());
    }
#endif
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
#include "enemy.cpp"
#include "error.cpp"
#include "fx/blood.cpp"
#include "fx/gem.cpp"
#include "fx/gold.cpp"
#include "fx/golden_chest.cpp"
#include "fx/goo.cpp"
#include "fx/number.cpp"
#include "fx/rainbow.cpp"
#include "game_client.cpp"
#include "game_server.cpp"
#include "healthbar.cpp"
#include "helpers.cpp"
#include "item_system.cpp"
#include "item_world.cpp"
#include "loot_table.cpp"
#include "monster/slime.cpp"
#include "net_client.cpp"
#include "net_message.cpp"
#include "net_server.cpp"
#include "net_stat.cpp"
#include "object.cpp"
#include "OpenSimplex2F.c"
#include "particles.cpp"
#include "perlin.cpp"
#include "player.cpp"
#include "server_cli.cpp"
#include "shadow.cpp"
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