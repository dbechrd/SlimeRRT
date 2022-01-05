#include "args.h"
#include "helpers.h"
#include <cstring>
#include <cstdio>

Args::Args(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++) {
        //printf("args[%d] = '%s'\n", i, argv[i]);

        if (!strcmp(argv[i], "-s")) {
            standalone = true;
            port = SV_DEFAULT_PORT;
            // TODO: Check if next arg is a port
        }
    }
}