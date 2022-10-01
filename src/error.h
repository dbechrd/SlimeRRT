#pragma once
#include "raylib/raylib.h"

enum class ErrorType {
    Success    = 0,  // No error occurred
    NotConnected,
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

#define E_INFO(format, ...) \
do { \
    TraceLog(LOG_INFO, "[%s]: " format, LOG_SRC, __VA_ARGS__); fflush(stdout); \
} while(0);

// TODO: Why the fuck doesn't LOG_WARNING work??
#define E_WARN(format, ...) \
do { \
    TraceLog(LOG_WARNING, "[%s]: " format, LOG_SRC, __VA_ARGS__); fflush(stdout); \
} while(0);

#define E_ERROR(err_code, format, ...) \
do { \
    ErrorType e__code = (err_code); \
    if (e__code != ErrorType::Success) { \
        TraceLog(LOG_ERROR, "[%s:%d]\n[%s][%s (%d)]: " format, \
            __FILE__, __LINE__, LOG_SRC, #err_code, (int)err_code, __VA_ARGS__); \
        fflush(stdout); \
        return (err_code); \
    } \
} while(0);
