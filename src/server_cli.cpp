#include "item_catalog.h"
#include "loot_table.h"
#include "net_client.h"
#include "server_cli.h"
#include "dlb_types.h"

#include <chrono>
#include <future>

using namespace std::chrono_literals;

const char *ServerCLI::LOG_SRC = "ServerCLI";

ErrorType ServerCLI::Run(const char *serverHost, unsigned short serverPort)
{
    std::thread serverThread;

    const char *title = "Attack the slimes!";
    if (args.server) {
        title = "[Open to LAN] Attack the slimes!";
    }

    E_ASSERT(netClient.OpenSocket(), "Failed to open client socket");
    E_ASSERT(netClient.Connect(serverHost, serverPort, "admin", "abc"), "Failed to connect client");

    ItemCatalog::instance.Load();
    loot_table_init();

#if 0
    // TODO(dlb): Does the server CLI need to know about the world? Would be cool..
    World lobby{};
    tilemap_generate_lobby(&lobby.map);
    world = &lobby;
#endif
    bool running = true;

    serverThread = std::thread([&running, this] {
        while (running) {
            netClient.Receive();
        }
    });

    char command[80];
    while (running) {
        memset(command, 0, sizeof(command));
        fputs("> ", stdout);
        if (fgets(command, sizeof(command) - 1, stdin)) {
            // Ignore remainder of long line
            char *newline = (char *)memchr(command, '\n', sizeof(command));
            if (newline) {
                *newline = 0;
            } else {
                char c = 0;
                do { fgetc(stdin); } while (c != '\n');
            }

            // Parse command
            if (!strcmp(command, "hi")) {
                netClient.SendChatMessage(CSTR("Hi :)"));
            } else if (!strcmp(command, "exit")) {
                fputs("Goodbye.\n", stdout);
                running = false;
            } else if (!strcmp(command, "shutdown") || !strcmp(command, "quit")) {
                fputs("Did you mean 'exit'?\n", stdout);
            } else {
                fprintf(stdout, "Unrecognized command '%s'.\n", command);
            }
        }
    }

    serverThread.join();
    return ErrorType::Success;
}
