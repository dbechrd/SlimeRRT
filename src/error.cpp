#include "clock.h"
#include "error.h"

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <thread>
#include <mutex>

thread_local FILE *logFile;

static void traceLogCallback(int logType, const char *text, va_list args)
{
    const char *logTypeStr = "";
    switch (logType) {
        case LOG_TRACE  : logTypeStr = "[TRACE]"; break;
        case LOG_DEBUG  : logTypeStr = "[DEBUG]"; break;
        case LOG_INFO   : logTypeStr = "[ INFO]"; break;
        case LOG_WARNING: logTypeStr = "[ WARN]"; break;
        case LOG_ERROR  : logTypeStr = "[ERROR]"; break;
        case LOG_FATAL  : logTypeStr = "[FATAL]"; break;
    }

    thread_local std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    double time = 0;
#if 1
    // Raylib CloseWindow() kills glfw then tries to log something.. which means the GLFW timer is invalid -.-
    // If there was ever a window, and now the window is gone (i.e. glfw dead), don't request a timestamp)
    thread_local bool windowDetected = 0;
    if (!windowDetected || IsWindowReady()) {
        time = glfwGetTime();
        windowDetected = IsWindowReady();
    }
#else
    time = g_clock.now;
#endif

    if (logFile) {
        fprintf(logFile, "[%0.3fs] %s ", time, logTypeStr);
        vfprintf(logFile, text, args);
        fputs("\n", logFile);
        fflush(logFile);
    }

    fprintf(stdout, "[%0.3fs] %s ", time, logTypeStr);
    vfprintf(stdout, text, args);
    fputs("\n", stdout);
    fflush(stdout);

    //if (logType == LOG_FATAL) {
    //    assert(!"Catch in debugger");
    //    if (logFile) fclose(logFile);
    //    //UNUSED(getc(stdin));
    //    //exit(-1);
    //}
}

void error_init(const char *logPath)
{
    if (logFile) return;

    logFile = fopen(logPath, "w");
    SetTraceLogLevel(LOG_DEBUG);
    SetTraceLogCallback(traceLogCallback);
}

void error_free()
{
    if (!logFile) return;

    fclose(logFile);
    logFile = 0;
}