#pragma once
#include "raylib/raylib.h"

enum class ErrorType {
    Success    = 0,  // No error occurred
    AssertFailed,
    NotConnected,
    NotFound,
    Overflow,
    OutOfBounds,
    AllocFailed_Full,       // Container is already full
    AllocFailed_Duplicate,  // ID already in use
    FileWriteFailed,
    FileReadFailed,
    ENetInitFailed,
    ENetServiceError,
    HostCreateFailed,
    PacketCreateFailed,
    PeerConnectFailed,
    PeerSendFailed,
    PlayerNotFound,
    EnemyNotFound,
    PancakeVerifyFailed,
    ServerFull,
    UserAccountInUse,
    Count
};

extern const char *g_err_msg[(int)ErrorType::Count];

void error_init(const char *logPath);
void error_free();

//#define E_START ErrorType e__code = ErrorType::Success;
//#define E_CLEANUP e_cleanup:
//#define E_END return e__code;
//#define E_END_INT return (int)e__code;
//#define E_CLEAN_END E_CLEANUP E_END;

#define E__LOG(level, format, ...) \
    do { \
        TraceLog(level, "%-11s] %s" format, LOG_SRC, g_clock.server \
            ? "[S] " \
            : "[C] ", __VA_ARGS__); \
        fflush(stdout); \
    } while(0)

// Standard log msg level helpers
#define E_TRACE(format, ...) E__LOG(LOG_NONE + LOG_TRACE, format, __VA_ARGS__)
#define E_DEBUG(format, ...) E__LOG(LOG_NONE + LOG_DEBUG, format, __VA_ARGS__)
#define E_INFO(format, ...) E__LOG(LOG_NONE + LOG_INFO, format, __VA_ARGS__)
#define E_WARN(format, ...) E__LOG(LOG_NONE + LOG_WARNING, format, __VA_ARGS__)

// Log debug msg if expr is true/not null
#define E_DEBUG_TRUE(expr, format, ...) \
    do { \
        if (expr) { \
            E__LOG(LOG_NONE + LOG_DEBUG, format, __VA_ARGS__); \
        } \
    } while(0)

// Log debug msg if expr is false/null
#define E_DEBUG_FALSE(expr, format, ...) E_DEBUG_TRUE(!(expr), format, __VA_ARGS__)
#define E_DEBUG_NULL E_DEBUG_FALSE

// Log error w/ file/line info and error code, iff err is non-success
#define E_ERROR(expr, format, ...) \
    do { \
        ErrorType e_error = (expr); \
        if (e_error != ErrorType::Success) { \
            E__LOG(LOG_NONE + LOG_ERROR, "%s (%d)\n  %s:%d\n  " format, #expr, (int)e_error, __FILE__, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

// Log error w/ file/line info and error code, iff err is non-success
#define E_ERROR_RETURN(expr, format, ...) \
    do { \
        ErrorType e_error_return = (expr); \
        if (e_error_return != ErrorType::Success) { \
            E_ERROR(e_error_return, format, __VA_ARGS__); \
            return e_error_return; \
        } \
    } while(0)
