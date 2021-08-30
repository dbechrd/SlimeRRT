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

#define DLB_RAND_IMPLEMENTATION
#include "dlb_rand.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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