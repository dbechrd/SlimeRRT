#pragma once
#include "raylib/raylib.h"

enum class ErrorType {
    Success    = 0,  // No error occurred
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

#define E_START ErrorType e__code = ErrorType::Success; {
#define E_CLEANUP } e_cleanup:
#define E_END return e__code;
#define E_END_INT return (int)e__code;
#define E_CLEAN_END E_CLEANUP E_END;

#define E_FATAL(err_code, format, ...) \
do { \
    TraceLog(LOG_FATAL, "[%s:%d]\n[%s][%s (%d)]: " format "\n", \
        __FILE__, __LINE__, LOG_SRC, #err_code, (int)err_code, __VA_ARGS__); \
    e__code = (err_code); \
    goto e_cleanup; \
} while(0);

#define E_INFO(format, ...) \
do { \
    TraceLog(LOG_INFO, "[%s]: " format "\n", LOG_SRC, __VA_ARGS__); \
} while(0);

//#define E_ERROR(err_code, format, ...) \
//    E_LOG_TYPE(LOG_ERROR, err_code, format, __VA_ARGS__);

#define E_CHECK(err_code, format, ...) \
    e__code = (err_code); \
    if (e__code != ErrorType::Success) { \
        E_FATAL(e__code, format, __VA_ARGS__); \
    }

//#define E_CHECK_ALLOC(cond, format, ...) \
//    if (!(cond)) { \
//        E_FATAL(ErrorType::AllocFailed, format, __VA_ARGS__); \
//    }