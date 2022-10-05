#include "clock.h"
#include "error.h"

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <thread>
#include <mutex>

thread_local FILE *logFile;

#define ESC "\033["
#define RESET ESC "0m"

// Font decorations
#define STY_BOLD       ESC "1m"
#define STY_DIM        ESC "2m"
#define STY_ITALIC     ESC "3m"
#define STY_UNDERLINE  ESC "4m"
#define STY_BLINK      ESC "5m"  // doesn't work
#define STY_INVERT     ESC "7m"  // inverts fg and bg colors
#define STY_HIDDEN     ESC "8m"
#define STY_STRIKE     ESC "9m"

// Background colors
#define BG_BLACK       ESC "40m"
#define BG_RED         ESC "41m"
#define BG_GREEN       ESC "42m"
#define BG_YELLOW      ESC "43m"
#define BG_BLUE        ESC "44m"
#define BG_MAGENTA     ESC "45m"
#define BG_CYAN        ESC "46m"
#define BG_WHITE       ESC "47m"
#define BG_DEFAULT     ESC "49m"

// Foreground colors
#define FG_BLACK       ESC "30m"
#define FG_RED         ESC "31m"
#define FG_GREEN       ESC "32m"
#define FG_YELLOW      ESC "33m"
#define FG_BLUE        ESC "34m"
#define FG_MAGENTA     ESC "35m"
#define FG_CYAN        ESC "36m"
#define FG_WHITE       ESC "37m"
#define FG_DEFAULT     ESC "39m"

static void traceLogCallback(int logType, const char *text, va_list args)
{
#if 0
    printf(FG_RED     "FG_RED     \n" RESET);
    printf(FG_GREEN   "FG_GREEN   \n" RESET);
    printf(FG_YELLOW  "FG_YELLOW  \n" RESET);
    printf(FG_BLUE    "FG_BLUE    \n" RESET);
    printf(FG_MAGENTA "FG_MAGENTA \n" RESET);
    printf(FG_CYAN    "FG_CYAN    \n" RESET);
    printf(FG_WHITE   "FG_WHITE   \n" RESET);

    printf(FG_RED     STY_ITALIC "FG_RED     ITAL \n" RESET);
    printf(FG_GREEN   STY_ITALIC "FG_GREEN   ITAL \n" RESET);
    printf(FG_YELLOW  STY_ITALIC "FG_YELLOW  ITAL \n" RESET);
    printf(FG_BLUE    STY_ITALIC "FG_BLUE    ITAL \n" RESET);
    printf(FG_MAGENTA STY_ITALIC "FG_MAGENTA ITAL \n" RESET);
    printf(FG_CYAN    STY_ITALIC "FG_CYAN    ITAL \n" RESET);
    printf(FG_WHITE   STY_ITALIC "FG_WHITE   ITAL \n" RESET);

    printf(FG_RED     STY_UNDERLINE "FG_RED     UNDER \n" RESET);
    printf(FG_GREEN   STY_UNDERLINE "FG_GREEN   UNDER \n" RESET);
    printf(FG_YELLOW  STY_UNDERLINE "FG_YELLOW  UNDER \n" RESET);
    printf(FG_BLUE    STY_UNDERLINE "FG_BLUE    UNDER \n" RESET);
    printf(FG_MAGENTA STY_UNDERLINE "FG_MAGENTA UNDER \n" RESET);
    printf(FG_CYAN    STY_UNDERLINE "FG_CYAN    UNDER \n" RESET);
    printf(FG_WHITE   STY_UNDERLINE "FG_WHITE   UNDER \n" RESET);

    printf(FG_RED     STY_ITALIC STY_UNDERLINE STY_BLINK "FG_RED     ITAL UNDER BLINK \n" RESET);
    printf(FG_GREEN   STY_ITALIC STY_UNDERLINE STY_BLINK "FG_GREEN   ITAL UNDER BLINK \n" RESET);
    printf(FG_YELLOW  STY_ITALIC STY_UNDERLINE STY_BLINK "FG_YELLOW  ITAL UNDER BLINK \n" RESET);
    printf(FG_BLUE    STY_ITALIC STY_UNDERLINE STY_BLINK "FG_BLUE    ITAL UNDER BLINK \n" RESET);
    printf(FG_MAGENTA STY_ITALIC STY_UNDERLINE STY_BLINK "FG_MAGENTA ITAL UNDER BLINK \n" RESET);
    printf(FG_CYAN    STY_ITALIC STY_UNDERLINE STY_BLINK "FG_CYAN    ITAL UNDER BLINK \n" RESET);
    printf(FG_WHITE   STY_ITALIC STY_UNDERLINE STY_BLINK "FG_WHITE   ITAL UNDER BLINK \n" RESET);
#endif

    const char *escCode = "";
    const char *logLevel = "";
    switch (logType) {
        case LOG_TRACE   : escCode = STY_BOLD   FG_MAGENTA ; logLevel = "RL_TRACE"; break;
        case LOG_DEBUG   : escCode = STY_BOLD   FG_CYAN    ; logLevel = "RL_DEBUG"; break;
        case LOG_INFO    : escCode = STY_DIM    FG_WHITE   ; logLevel = "RL_INFO "; break;
        case LOG_WARNING : escCode = STY_BOLD   FG_YELLOW  ; logLevel = "RL_WARN "; break;
        case LOG_ERROR   : escCode = STY_BOLD   FG_RED     ; logLevel = "RL_ERROR"; break;
        case LOG_FATAL   : escCode = STY_ITALIC FG_RED     ; logLevel = "RL_FATAL"; break;
        case LOG_NONE + LOG_TRACE   : escCode = STY_BOLD   FG_MAGENTA ; logLevel = "TRACE"; break;
        case LOG_NONE + LOG_DEBUG   : escCode = STY_BOLD   FG_CYAN    ; logLevel = "DEBUG"; break;
        case LOG_NONE + LOG_INFO    : escCode = STY_DIM    FG_WHITE   ; logLevel = "INFO "; break;
        case LOG_NONE + LOG_WARNING : escCode = STY_BOLD   FG_YELLOW  ; logLevel = "WARN "; break;
        case LOG_NONE + LOG_ERROR   : escCode = STY_BOLD   FG_RED     ; logLevel = "ERROR"; break;
        case LOG_NONE + LOG_FATAL   : escCode = STY_ITALIC FG_RED     ; logLevel = "FATAL"; break;
    }

    static std::mutex mutex;
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
        fprintf(logFile, "[%6.3fs|%-8s| ", time, logLevel);
        vfprintf(logFile, text, args);
        fputs("\n", logFile);
        fflush(logFile);
    }

    fprintf(stdout, "%s[%6.3fs|%-8s|", escCode, time, logLevel);
    vfprintf(stdout, text, args);
    fputs("\n" RESET, stdout);
    fflush(stdout);

    //if (logType == LOG_ERROR) {
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
#if 0
    SetTraceLogLevel(LOG_TRACE);
#else
    SetTraceLogLevel(LOG_DEBUG);
#endif
    SetTraceLogCallback(traceLogCallback);
}

void error_free()
{
    if (!logFile) return;

    fclose(logFile);
    logFile = 0;
}