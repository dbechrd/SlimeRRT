#pragma once
#include "error.h"
#include "fbs.h"
#include "servers_generated.h"

// TODO: Should the base multiplayer menu own this data?
struct ServerDB {
    FBS_Buffer          file{};  // raw file buffer
    const DB::ServerDB *flat{};  // read-only flatbuffer

                   ~ServerDB (void);
    ErrorType      Load      (const char *filename);
    void           Unload    (void);
    ErrorType      Save      (const char *filename = 0);
    size_t         Size      (void);
    DB::ServerDBT *Data      (void);
    ErrorType      Add       (const DB::ServerT &server);
    ErrorType      Replace   (size_t index, const DB::ServerT &server);
    ErrorType      Delete    (size_t index);

private:
    const char *LOG_SRC = "ServerDB";

    const char    *filename {};  // name of last loaded file
    DB::ServerDBT *native   {};  // mutable native data structure
} server_db{};