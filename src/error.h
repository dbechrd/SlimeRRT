#pragma once
#include "raylib/raylib.h"

enum class ErrorType {
    Success    = 0,  // No error occurred
    NotConnected,
    AllocFailed,     // Failed to allocate memory
    ENetInitFailed,
    HostCreateFailed,
    PacketCreateFailed,
    PeerConnectFailed,
    PeerSendFailed,
    Count
};

extern const char *g_err_msg[(int)ErrorType::Count];

void error_init();

//#define E_START ErrorType e__code = ErrorType::Success;
//#define E_CLEANUP e_cleanup:
//#define E_END return e__code;
//#define E_END_INT return (int)e__code;
//#define E_CLEAN_END E_CLEANUP E_END;

#define E_ASSERT(err_code, format, ...) \
do { \
    ErrorType e__code = (err_code); \
    if (e__code != ErrorType::Success) { \
        TraceLog(LOG_FATAL, "[FATAL][%s:%d]\n[%s][%s (%d)]: " format, \
            __FILE__, __LINE__, LOG_SRC, #err_code, (int)err_code, __VA_ARGS__); \
        fflush(stdout); \
        return (err_code); \
    } \
} while(0);

#define E_INFO(format, ...) \
do { \
    TraceLog(LOG_INFO, "[INFO][%s]: " format, LOG_SRC, __VA_ARGS__); fflush(stdout); \
} while(0);

// TODO: Why the fuck doesn't LOG_WARNING work??
#define E_WARN(format, ...) \
do { \
    TraceLog(LOG_INFO, "[WARN][%s]: " format, LOG_SRC, __VA_ARGS__); fflush(stdout); \
} while(0);
