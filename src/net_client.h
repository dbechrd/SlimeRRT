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
    uint32_t        inputSeq        {};  // seq # of input last sent to server
    RingBuffer<InputSnapshot, SERVER_INPUT_HISTORY> inputHistory {};
    RingBuffer<WorldSnapshot, SERVER_WORLD_HISTORY> worldHistory {};

    NetClient::~NetClient();
    ErrorType OpenSocket();
    ErrorType Connect(const char *serverHost, unsigned short serverPort, const char *user, const char *password);
    ErrorType SendChatMessage(const char *message, size_t messageLength);
    ErrorType SendPlayerInput();
    void      PredictPlayer();
    void      ReconcilePlayer();
    void      Interpolate(double renderAt);
    ErrorType Receive();
    void Disconnect();
    void CloseSocket();

private:
    static const char *LOG_SRC;
    NetMessage netMsg {};

    ErrorType   SendRaw         (const void *data, size_t size);
    ErrorType   SendMsg         (NetMessage &message);
    ErrorType   Auth            (void);
    bool        InterpolateBody (Body3D &body, double renderAt);
    void        ProcessMsg      (ENetPacket &packet);
    const char *ServerStateString();
};