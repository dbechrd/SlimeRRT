#include "args.h"
#include <cstring>
#include <cstdio>

Args::Args(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++) {
        printf("args[%d] = '%s'\n", i, argv[i]);

        if (!strcmp(argv[i], "-s")) {
            server = true;
            // TODO: Check if next arg is a port
        }
    }
}