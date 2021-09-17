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

#include "args.h"
#include "error.h"
#include "game_client.h"
#include "game_server.h"
#include "zed_net.h"
#include "../test/tests.h"

#include <future>

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
        TraceLog(LOG_FATAL, "Failed to initialize network utilities (zed). Error: %s\n", err);
    }

    int enet_code = enet_initialize();
    if (enet_code < 0) {
        TraceLog(LOG_FATAL, "Failed to initialize network utilities (enet). Error code: %d\n", enet_code);
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
    enet_deinitialize();
    zed_net_shutdown();
    return 0;
}

#pragma warning(push)
#pragma warning(disable:26819)
#define DLB_MURMUR3_IMPLEMENTATION
#include "dlb_murmur3.h"
#pragma warning(pop)

#define DLB_RAND_IMPLEMENTATION
#include "dlb_rand.h"

#define RAYGUI_IMPLEMENTATION
#include "raylib/raygui.h"

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
#include "drawable.cpp"
#include "draw_command.cpp"
#include "error.cpp"
#include "game_client.cpp"
#include "game_server.cpp"
#include "healthbar.cpp"
#include "helpers.cpp"
#include "item.cpp"
#include "item_catalog.cpp"
#include "loot_table.h"
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