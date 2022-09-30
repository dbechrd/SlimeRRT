#pragma once
#include "chat.h"
#include "controller.h"
#include "fbs.h"
#include "dlb_types.h"

struct World;

struct NetClient {
    char     serverHost       [HOSTNAME_LENGTH_MAX]{};
    size_t   serverHostLength {};
    uint16_t serverPort       {};
    size_t   usernameLength   {};
    char     username         [USERNAME_LENGTH_MAX]{};
    size_t   passwordLength   {};
    char     password         [PASSWORD_LENGTH_MAX]{};

    uint32_t connectionToken  {};
    ENetHost *client          {};
    ENetPeer *server          {};
    World    *serverWorld     {};
    double   lastInputSentAt  {};

    uint32_t inputSeq         {};  // seq # of input last sent to server
    RingBuffer<InputSample,   CL_INPUT_HISTORY> inputHistory {};
    RingBuffer<WorldSnapshot, CL_WORLD_HISTORY> worldHistory {};

    NetClient                     (void);
    ~NetClient                    (void);
    ErrorType OpenSocket          (void);
    ErrorType Connect             (const char *serverHost, unsigned short serverPort, const char *user, const char *password);
    ErrorType SendChatMessage     (const char *message, size_t messageLength);
    ErrorType SendSlotClick       (int slot, bool doubleClicked);
    ErrorType SendSlotScroll      (int slot, int scrollY);
    ErrorType SendSlotDrop        (int slot, uint32_t count);
    ErrorType SendTileInteract    (float worldX, float worldY);
    ErrorType SendPlayerInput     (void);
    void      PredictPlayer       (void);
    void      ReconcilePlayer     (void);
    ErrorType Receive             (void);
    bool      IsConnecting        (void) const;
    bool      IsConnected         (void) const;
    bool      IsDisconnected      (void) const;
    void      Disconnect          (void);
    void      CloseSocket         (void);
    bool      ConnectedAndSpawned (void) const;

    FBS_Buffer &FBS_Servers(void) {
        return fbs_servers;
    }

private:
    static const char *LOG_SRC;
    NetMessage tempMsg {};
    ENetBuffer rawPacket {};
    FBS_Buffer fbs_servers {};

    ErrorType SaveServerDB(const char *filename);
    ErrorType LoadServerDB(const char *filename);

    ErrorType   SendRaw           (const void *data, size_t size);
    ErrorType   SendMsg           (NetMessage &message);
    ErrorType   Auth              (void);
    void        ProcessMsg        (ENetPacket &packet);
    const char *ServerStateString (void);
};