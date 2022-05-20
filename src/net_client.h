#pragma once
#include "chat.h"
#include "controller.h"
#include "dlb_types.h"

struct World;

struct NetClient {
    const char     *serverHost      {};
    unsigned short  serverPort      {};
    ENetHost       *client          {};
    ENetPeer       *server          {};
    uint32_t        connectionToken {};
    size_t          usernameLength  {};
    char            username        [USERNAME_LENGTH_MAX] {};
    size_t          passwordLength  {};
    char            password        [PASSWORD_LENGTH_MAX] {};
    World          *serverWorld     {};
    //uint32_t        inputSeq        {};  // seq # of input last sent to server
    RingBuffer<InputSample,   CL_INPUT_HISTORY> inputHistory {};
    RingBuffer<WorldSnapshot, CL_WORLD_HISTORY> worldHistory {};

    ~NetClient();
    ErrorType OpenSocket      (void);
    ErrorType Connect         (const char *serverHost, unsigned short serverPort, const char *user, const char *password);
    ErrorType SendChatMessage (const char *message, size_t messageLength);
    ErrorType SendSlotClick   (int slot, bool doubleClicked);
    ErrorType SendSlotScroll  (int slot, int scrollY);
    ErrorType SendPlayerInput (void);
    void      PredictPlayer   (void);
    void      ReconcilePlayer (double tickDt);
    ErrorType Receive         (void);
    bool      IsConnecting    (void) const;
    bool      IsConnected     (void) const;
    bool      IsDisconnected  (void) const;
    void      Disconnect      (void);
    void      CloseSocket     (void);

private:
    static const char *LOG_SRC;
    NetMessage tempMsg {};

    ErrorType   SendRaw           (const void *data, size_t size);
    ErrorType   SendMsg           (NetMessage &message);
    ErrorType   Auth              (void);
    void        ProcessMsg        (ENetPacket &packet);
    const char *ServerStateString (void);
};