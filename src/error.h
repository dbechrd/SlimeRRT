#pragma once
#include "raylib.h"

enum class ErrorType {
    Success        = 0,  // No error occurred
    AllocFailed    = 1,  // Failed to allocate memory
    NetPortInUse   = 2,  // Network port already in use
    SockRecvFailed = 3,  // Failed to receive data via socket
    SockSendFailed = 4,  // Failed to send data via socket
    Count
};

extern const char *g_err_msg[(int)ErrorType::Count];

void error_init();

#define E_START ErrorType e_code = ErrorType::Success;
#define E_CLEANUP e_cleanup:
#define E_END return e_code;
#define E_END_INT return (int)e_code;
#define E_CLEAN_END E_CLEANUP E_END;

#define E_LOG_TYPE(log_type, err_code, format, ...) \
do { \
    TraceLog((log_type), "[%s:%d]\n[%s][%s (%d)]: " format "\n", \
        __FILE__, __LINE__, LOG_SRC, #err_code, (int)err_code, __VA_ARGS__); \
    e_code = (err_code); \
    goto e_cleanup; \
} while(0);

#define E_FATAL(err_code, format, ...) \
    E_LOG_TYPE(LOG_FATAL, err_code, format, __VA_ARGS__);

//#define E_ERROR(err_code, format, ...) \
//    E_LOG_TYPE(LOG_ERROR, err_code, format, __VA_ARGS__);

#define E_CHECK(cond, format, ...) \
    e_code = (cond); \
    if (e_code != ErrorType::Success) { \
        E_FATAL(e_code, format, __VA_ARGS__); \
    }

#define E_CHECK_ALLOC(cond, format, ...) \
    if (!(cond)) { \
        E_FATAL(ErrorType::AllocFailed, format, __VA_ARGS__); \
    }