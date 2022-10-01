#pragma once
#include "raylib/raylib.h"

enum class ErrorType {
    Success    = 0,  // No error occurred
    AssertFailed,
    NotConnected,
    NotFound,
    Overflow,
    OutOfBounds,
    AllocFailed,     // Failed to allocate memory
    FileWriteFailed,
    FileReadFailed,
    ENetInitFailed,
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
    TraceLog(level, "[%s][%c]: " format, LOG_SRC, g_clock.server ? 'S' : 'C', __VA_ARGS__); \
    fflush(stdout); \
} while(0);

#define E_TRACE(format, ...) E__LOG(LOG_TRACE, format, __VA_ARGS__)
#define E_DEBUG(format, ...) E__LOG(LOG_DEBUG, format, __VA_ARGS__)
#define E_INFO(format, ...) E__LOG(LOG_INFO, format, __VA_ARGS__)
#define E_WARN(format, ...) E__LOG(LOG_WARNING, format, __VA_ARGS__)

#define E_ERROR(err_code, format, ...) \
    E__LOG(LOG_ERROR, "%s (%d)\n  %s:%d\n  " format, #err_code, (int)(err_code), __FILE__, __LINE__, __VA_ARGS__);

#define E_CHECKMSG(err_code, format, ...) \
    do { \
        if ((err_code) != ErrorType::Success) { \
            E_ERROR((err_code), format, __VA_ARGS__); \
            return (err_code); \
        } \
    } while(0);

#if 0
#define E_CHECK(err_code) \
    do { \
        if ((err_code) != ErrorType::Success) { \
            E_ERROR((err_code), ""); \
            return (err_code); \
        } \
    } while(0);
#endif