#include "raylib.h"

enum {
    E_SUCCESS           = 0,  // No error occurred
    E_ALLOC_FAILED      = 1,  // Failed to allocate memory
    E_NET_PORT_IN_USE   = 2,  // Network port already in use
    E_SOCK_RECV_FAILED  = 3,  // Failed to receive data via socket
    E_SOCK_SEND_FAILED  = 4,  // Failed to send data via socket
    E_COUNT
};

extern const char *g_err_msg[E_COUNT];

void error_init();

#define E_START int e_code = E_SUCCESS;
#define E_CLEANUP e_cleanup:
#define E_END return e_code;
#define E_CLEAN_END E_CLEANUP E_END;

#define E_LOG_TYPE(log_type, err_code, format, ...) \
do { \
    TraceLog((log_type), "[%s:%d][%s] %s (%d): ", __FILE__, __LINE__, LOG_SRC, #err_code, (err_code)); \
    TraceLog((log_type), (format), __VA_ARGS__); \
    TraceLog((log_type), "\n"); \
    e_code = (err_code); \
    goto e_cleanup; \
} while(0);

#define E_FATAL(err_code, format, ...) \
    E_LOG_TYPE(LOG_FATAL, (err_code), (format), __VA_ARGS__);

#define E_ERROR(err_code, format, ...) \
    E_LOG_TYPE(LOG_ERROR, (err_code), (format), __VA_ARGS__);

#define E_CHECK(cond) \
    e_code = (cond); \
    if (e_code) { \
        goto e_cleanup; \
    }

#define E_CHECK_ALLOC(cond, format, ...) \
    if (!(cond)) { \
        E_FATAL(E_ALLOC_FAILED, (format), __VA_ARGS__); \
    }