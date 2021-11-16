#if 1
// -- windows.h flags --
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define OWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NOIME
// -- mmsystem.h flags --
#define MMNODRV
#define MMNOSOUND
#define MMNOWAVE
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
//#define MMNOTIMER
#define MMNOJOY
#define MMNOMCI
#define MMNOMMIO
#define MMNOMMSYSTEM
// -- winsock2.h flags --
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4245)
#pragma warning(disable:4701)
#pragma warning(disable:4996)
#pragma warning(disable:6385)
#pragma warning(disable:26812)
#define ENET_IMPLEMENTATION
#include "enet_zpl.h"
#pragma warning(pop)
#undef far
#undef near
#endif

#include "args.h"
#include "error.h"
#include "game_client.h"
#include "game_server.h"
#include "server_cli.h"
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
            GameServer gameServer{ args };
            gameServer.Run();
        }
    });

    if (args.server) {
        // TODO: Make CLI not be an entire client/player. Makes no sense for the CLI to show up in the world LUL.
        //ServerCLI serverCli{ args };
        //serverCli.Run("localhost", SERVER_PORT);
        //AllocConsole();
    } else {
#ifndef DEBUG
        FreeConsole();
#endif
        GameClient gameClient{ args };
        //gameClient.Run("slime.theprogrammingjunkie.com", SERVER_PORT);
        //gameClient.Run("127.0.0.1", SERVER_PORT);
        gameClient.Run("localhost", SERVER_PORT);
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
#pragma warning(pop)

#define DLB_RAND_IMPLEMENTATION
#include "dlb_rand.h"

#pragma warning(push)
#pragma warning(disable:6031)  // Return value ignored
#pragma warning(disable:26812)
#define RAYGUI_IMPLEMENTATION
#include "raylib/raygui.h"
#pragma warning(pop)

#define GUI_TEXTBOX_EXTENDED_IMPLEMENTATION
#include "gui_textbox_extended.h"

#if 0
#pragma warning(push)
#pragma warning(disable: 4244)  // conversion from 'int' to 'float'
#pragma warning(disable: 4267)  // conversion from 'size_t' to 'int'
#define GUI_TEXTBOX_EXTENDED_IMPLEMENTATION
#include "gui_textbox_extended.h"
#pragma warning(pop)
#endif

#include "args.cpp"
#include "bit_stream.cpp"
#include "body.cpp"
#include "chat.cpp"
#include "controller.cpp"
#include "draw_command.cpp"
#include "error.cpp"
#include "game_client.cpp"
#include "game_server.cpp"
#include "healthbar.cpp"
#include "helpers.cpp"
#include "item.cpp"
#include "item_catalog.cpp"
#include "loot_table.cpp"
#include "maths.cpp"
#include "net_client.cpp"
#include "net_server.cpp"
#include "net_message.cpp"
#include "packet.cpp"
#include "particles.cpp"
#include "particle_fx_blood.cpp"
#include "particle_fx_gold.cpp"
#include "particle_fx_goo.cpp"
#include "player.cpp"
#include "player_inventory.cpp"
#include "server_cli.cpp"
#include "shadow.cpp"
#include "slime.cpp"
#include "sound_catalog.cpp"
#include "sprite.cpp"
#include "spritesheet.cpp"
#include "spritesheet_catalog.cpp"
#include "tilemap.cpp"
#include "tileset.cpp"
#include "world.cpp"
#include "../test/tests.cpp"