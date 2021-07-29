#include "raylib.h"

enum {
    E_SUCCESS           = 0,  // No error occurred
    E_ALLOC_FAILED      = 1,  // Failed to allocate memory
    E_NET_PORT_IN_USE   = 2,  // Network port already in use
    E_COUNT
};

extern const char *g_err_msg[E_COUNT];

void error_init();

#define E_START int e_code = E_SUCCESS;
#define E_CLEANUP e_cleanup:
#define E_END return e_code;
#define E_CLEAN_END E_CLEANUP E_END;

#define E_FATAL(err_code, format, ...) \
do { \
    TraceLog(LOG_FATAL, "[%s:%d][%s] %s (%d): ", __FILE__, __LINE__, LOG_SRC, #err_code, (err_code)); \
    TraceLog(LOG_FATAL, (format), __VA_ARGS__); \
    TraceLog(LOG_FATAL, "\n"); \
    e_code = (err_code); \
    goto e_cleanup; \
} while(0);

#define E_CHECK(err_code) \
    if (err_code) { \
        e_code = (err_code); \
        goto e_cleanup; \
    }

#define E_CHECK_ALLOC(cond, format, ...) \
    if (!(cond)) { \
        E_FATAL(E_ALLOC_FAILED, (format), __VA_ARGS__); \
    }