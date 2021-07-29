#include "error.h"

#include <cstdio>
#include <cassert>
#include <cstdlib>

static FILE *logFile;

static void traceLogCallback(int logType, const char *text, va_list args)
{
    vfprintf(logFile, text, args);
    fputs("\n", logFile);
    vfprintf(stdout, text, args);
    fputs("\n", stdout);
    fflush(logFile);
    if (logType >= LOG_WARNING) {
        fclose(logFile);
        assert(!"Catch in debugger");
        exit(-1);
    }
}

void error_init()
{
    logFile = fopen("log.txt", "w");
    SetTraceLogCallback(traceLogCallback);
}

void error_free()
{
    fclose(logFile);
}