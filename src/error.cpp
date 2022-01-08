#include "error.h"

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <thread>
#include <mutex>

static FILE *logFile;
static std::thread::id logFileOwner;
static thread_local const char *threadName = "unnamed";

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

    double time = 0;
    if (IsWindowReady()) {
        time = GetTime();
    }

    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    fprintf(logFile, "[%0.3fs][%6s]%s ", time, threadName, logTypeStr);
    vfprintf(logFile, text, args);
    fputs("\n", logFile);

    fprintf(stdout, "[%0.3fs][%6s]%s ", time, threadName, logTypeStr);
    vfprintf(stdout, text, args);
    fputs("\n", stdout);

    fflush(logFile);
    fflush(stdout);
    if (logType == LOG_FATAL) {
        fclose(logFile);
        assert(!"Catch in debugger");
        //UNUSED(getc(stdin));
        //exit(-1);
    }
}

void error_init()
{
    logFileOwner = std::this_thread::get_id();
    logFile = fopen("log.txt", "w");
    SetTraceLogCallback(traceLogCallback);
}

void error_set_thread_name(const char *name)
{
    threadName = name;
}

void error_free()
{
    fclose(logFile);
}